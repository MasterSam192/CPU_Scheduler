#include <stdio.h>
#include <string.h>
#include "engine.h"

#define MAX_TICKS 100000
#define IDLE_PID 0

typedef enum { ST_NEW, ST_READY, ST_RUNNING, ST_WAITING_IO, ST_TERMINATED } State;

typedef struct {
    Process def;
    int burst_index;
    int remaining_time;
    State state;
    int io_end_time;
    int current_priority;
    int wait_timer;
    int total_waiting_time;
    int response_time;
    int completion_time;
} RunState;

const char *policy_name(Policy p) {
    switch (p) {
        case POLICY_FCFS: return "FCFS";
        case POLICY_SJF_PREDICTED: return "SJF (predicted burst)";
        case POLICY_RR_ADAPTIVE: return "Round Robin (adaptive quantum)";
        case POLICY_PRIORITY_AGING: return "Priority (with aging)";
    }
    return "?";
}

static void rq_push_back(int *rq, int *count, int val) {
    rq[(*count)++] = val;
}

static int rq_remove_at(int *rq, int *count, int pos) {
    int val = rq[pos];
    for (int i = pos; i < *count - 1; i++) rq[i] = rq[i + 1];
    (*count)--;
    return val;
}

static int rq_pop_front(int *rq, int *count) {
    return rq_remove_at(rq, count, 0);
}

/* Returns the position (not the RunState index) within rq[] of the best
 * candidate under a "smaller key wins, ties broken by arrival then pid" rule. */
static int select_best_position(RunState *rs, const int *rq, int count,
                                  double (*key_fn)(const RunState *)) {
    int best_pos = 0;
    double best_key = key_fn(&rs[rq[0]]);
    for (int i = 1; i < count; i++) {
        double k = key_fn(&rs[rq[i]]);
        const RunState *cand = &rs[rq[i]];
        const RunState *best = &rs[rq[best_pos]];
        if (k < best_key ||
            (k == best_key && (cand->def.arrival_time < best->def.arrival_time ||
                                (cand->def.arrival_time == best->def.arrival_time &&
                                 cand->def.pid < best->def.pid)))) {
            best_key = k;
            best_pos = i;
        }
    }
    return best_pos;
}

static double key_predicted(const RunState *r) { return r->def.predicted_burst[r->burst_index]; }
static double key_priority(const RunState *r) { return (double)r->current_priority; }

static void gantt_record(SimResult *res, int pid, int t) {
    if (res->gantt_count > 0 && res->gantt[res->gantt_count - 1].pid == pid &&
        res->gantt[res->gantt_count - 1].end == t) {
        res->gantt[res->gantt_count - 1].end = t + 1;
    } else if (res->gantt_count < MAX_GANTT_SEGMENTS) {
        res->gantt[res->gantt_count].pid = pid;
        res->gantt[res->gantt_count].start = t;
        res->gantt[res->gantt_count].end = t + 1;
        res->gantt_count++;
    }
}

SimResult run_simulation(const Process *procs, int n, Policy policy, const SimConfig *cfg) {
    SimResult res;
    memset(&res, 0, sizeof(res));
    res.policy = policy;
    res.policy_name = policy_name(policy);
    res.n = n;

    RunState rs[MAX_PROCESSES];
    for (int i = 0; i < n; i++) {
        rs[i].def = procs[i];
        rs[i].burst_index = 0;
        rs[i].remaining_time = procs[i].cpu_burst[0];
        rs[i].state = ST_NEW;
        rs[i].io_end_time = 0;
        rs[i].current_priority = procs[i].base_priority;
        rs[i].wait_timer = 0;
        rs[i].total_waiting_time = 0;
        rs[i].response_time = -1;
        rs[i].completion_time = 0;
    }

    int ready_q[MAX_PROCESSES];
    int rq_count = 0;

    int running_idx = -1;
    int quantum_remaining = 0;
    int last_running_pid = -1;
    int completed = 0;
    int t = 0;

    while (completed < n && t < MAX_TICKS) {
        /* a) arrivals */
        for (int i = 0; i < n; i++) {
            if (rs[i].state == ST_NEW && rs[i].def.arrival_time == t) {
                rs[i].state = ST_READY;
                rs[i].wait_timer = 0;
                rq_push_back(ready_q, &rq_count, i);
            }
        }
        /* b) I/O completions */
        for (int i = 0; i < n; i++) {
            if (rs[i].state == ST_WAITING_IO && rs[i].io_end_time == t) {
                rs[i].burst_index++;
                rs[i].remaining_time = rs[i].def.cpu_burst[rs[i].burst_index];
                rs[i].state = ST_READY;
                rs[i].wait_timer = 0;
                rq_push_back(ready_q, &rq_count, i);
            }
        }

        /* c) dispatch if CPU idle */
        if (running_idx == -1 && rq_count > 0) {
            int pos;
            switch (policy) {
                case POLICY_SJF_PREDICTED:
                    pos = select_best_position(rs, ready_q, rq_count, key_predicted);
                    running_idx = rq_remove_at(ready_q, &rq_count, pos);
                    break;
                case POLICY_PRIORITY_AGING:
                    pos = select_best_position(rs, ready_q, rq_count, key_priority);
                    running_idx = rq_remove_at(ready_q, &rq_count, pos);
                    break;
                case POLICY_RR_ADAPTIVE: {
                    long sum = 0;
                    for (int i = 0; i < rq_count; i++) sum += rs[ready_q[i]].remaining_time;
                    int avg = (rq_count > 0) ? (int)((sum + rq_count / 2) / rq_count) : cfg->min_quantum;
                    quantum_remaining = avg;
                    if (quantum_remaining < cfg->min_quantum) quantum_remaining = cfg->min_quantum;
                    if (quantum_remaining > cfg->max_quantum) quantum_remaining = cfg->max_quantum;
                    running_idx = rq_pop_front(ready_q, &rq_count);
                    break;
                }
                case POLICY_FCFS:
                default:
                    running_idx = rq_pop_front(ready_q, &rq_count);
                    break;
            }
            rs[running_idx].state = ST_RUNNING;
            if (rs[running_idx].response_time == -1)
                rs[running_idx].response_time = t - rs[running_idx].def.arrival_time;
            if (last_running_pid != -1 && last_running_pid != rs[running_idx].def.pid)
                res.context_switches++;
            last_running_pid = rs[running_idx].def.pid;
        }

        /* d) waiting/aging bookkeeping for everyone still queued */
        for (int i = 0; i < rq_count; i++) {
            RunState *r = &rs[ready_q[i]];
            r->total_waiting_time++;
            if (policy == POLICY_PRIORITY_AGING) {
                r->wait_timer++;
                if (r->wait_timer >= cfg->aging_interval) {
                    r->wait_timer = 0;
                    if (r->current_priority > 1) r->current_priority--;
                }
            }
        }

        /* e) execute one tick */
        if (running_idx != -1) {
            gantt_record(&res, rs[running_idx].def.pid, t);
            res.cpu_busy_ticks++;
            rs[running_idx].remaining_time--;
            if (policy == POLICY_RR_ADAPTIVE) quantum_remaining--;

            if (rs[running_idx].remaining_time == 0) {
                int last_burst = (rs[running_idx].burst_index == rs[running_idx].def.num_bursts - 1);
                if (last_burst) {
                    rs[running_idx].state = ST_TERMINATED;
                    rs[running_idx].completion_time = t + 1;
                    completed++;
                } else {
                    rs[running_idx].state = ST_WAITING_IO;
                    rs[running_idx].io_end_time = t + 1 + rs[running_idx].def.io_burst[rs[running_idx].burst_index];
                }
                running_idx = -1;
            } else if (policy == POLICY_RR_ADAPTIVE && quantum_remaining == 0) {
                rs[running_idx].state = ST_READY;
                rs[running_idx].wait_timer = 0;
                rq_push_back(ready_q, &rq_count, running_idx);
                running_idx = -1;
            }
        } else {
            gantt_record(&res, IDLE_PID, t);
        }

        t++;
    }

    res.total_time = t;
    for (int i = 0; i < n; i++) {
        ProcessResult *pr = &res.results[i];
        pr->pid = rs[i].def.pid;
        pr->arrival_time = rs[i].def.arrival_time;
        pr->completion_time = rs[i].completion_time;
        pr->turnaround_time = rs[i].completion_time - rs[i].def.arrival_time;
        pr->waiting_time = rs[i].total_waiting_time;
        pr->response_time = rs[i].response_time;
        int total_cpu = 0;
        for (int b = 0; b < rs[i].def.num_bursts; b++) total_cpu += rs[i].def.cpu_burst[b];
        pr->total_cpu_time = total_cpu;
    }

    double sum_w = 0, sum_t = 0, sum_r = 0;
    for (int i = 0; i < n; i++) {
        sum_w += res.results[i].waiting_time;
        sum_t += res.results[i].turnaround_time;
        sum_r += res.results[i].response_time;
    }
    res.avg_waiting_time = sum_w / n;
    res.avg_turnaround_time = sum_t / n;
    res.avg_response_time = sum_r / n;
    res.cpu_utilization_pct = res.total_time > 0 ? (100.0 * res.cpu_busy_ticks / res.total_time) : 0.0;
    res.throughput = res.total_time > 0 ? ((double)n / res.total_time) : 0.0;

    return res;
}
