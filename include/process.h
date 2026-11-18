#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 50
#define MAX_BURSTS 5

/* Static definition of a process: what the "workload" looks like.
 * A process alternates CPU bursts and I/O bursts, e.g.
 *   cpu_burst[0] -> io_burst[0] -> cpu_burst[1] -> io_burst[1] -> ... -> cpu_burst[num_bursts-1]
 * io_burst[num_bursts-1] is unused (process terminates after its last CPU burst).
 *
 * predicted_burst[i] is the scheduler's ESTIMATE of cpu_burst[i], computed with
 * exponential averaging BEFORE that burst is known to have run -- this is what
 * makes SJF/SRTF realistic instead of "cheating" by using the true burst time.
 */
typedef struct {
    int pid;
    int arrival_time;
    int base_priority;         /* lower value = higher priority */
    int num_bursts;
    int cpu_burst[MAX_BURSTS];
    int io_burst[MAX_BURSTS];
    double predicted_burst[MAX_BURSTS];
} Process;

/* Config for the prediction formula and the adaptive policies. */
typedef struct {
    double alpha;            /* exponential averaging weight, 0 < alpha <= 1 */
    double initial_guess;    /* tau0: predicted length of the very first CPU burst */
    int min_quantum;         /* Round Robin adaptive quantum lower bound */
    int max_quantum;         /* Round Robin adaptive quantum upper bound */
    int aging_interval;      /* ticks a process must wait before priority improves by 1 */
} SimConfig;

SimConfig default_config(void);

/* Fills predicted_burst[] for every process using exponential averaging over
 * that process's OWN historical CPU bursts. Independent of which scheduling
 * algorithm will later run the process, since burst lengths are intrinsic to
 * the workload, not to the scheduler. Must be called once after bursts/num_bursts
 * are set and before running any simulation. */
void compute_predicted_bursts(Process *procs, int n, const SimConfig *cfg);

/* Interactive manual entry of a process set. Returns number of processes entered. */
int input_processes_manual(Process *procs, int max_n);

/* Generates a random process set for quick demos. */
int generate_processes_random(Process *procs, int n, int seed);

void print_process_table(const Process *procs, int n);

#endif
