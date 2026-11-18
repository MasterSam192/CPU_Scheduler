#ifndef ENGINE_H
#define ENGINE_H

#include "process.h"

typedef enum {
    POLICY_FCFS,
    POLICY_SJF_PREDICTED,
    POLICY_RR_ADAPTIVE,
    POLICY_PRIORITY_AGING
} Policy;

#define MAX_GANTT_SEGMENTS 2000

typedef struct {
    int pid;
    int start;
    int end;
} GanttSegment;

typedef struct {
    int pid;
    int arrival_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
    int total_cpu_time;   /* sum of actual cpu bursts */
} ProcessResult;

typedef struct {
    Policy policy;
    const char *policy_name;

    GanttSegment gantt[MAX_GANTT_SEGMENTS];
    int gantt_count;

    ProcessResult results[MAX_PROCESSES];
    int n;

    int total_time;          /* makespan: last completion time */
    int cpu_busy_ticks;
    int context_switches;

    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
    double cpu_utilization_pct;
    double throughput;       /* processes completed per unit time */
} SimResult;

/* Runs the full tick-based simulation for the given process set under the
 * given scheduling policy. procs is NOT modified (deep-copied internally),
 * so the same workload can be re-run under every policy for a fair
 * side-by-side comparison. */
SimResult run_simulation(const Process *procs, int n, Policy policy, const SimConfig *cfg);

const char *policy_name(Policy p);

#endif
