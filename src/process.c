#include <stdio.h>
#include <stdlib.h>
#include "process.h"

SimConfig default_config(void) {
    SimConfig cfg;
    cfg.alpha = 0.5;
    cfg.initial_guess = 6.0;
    cfg.min_quantum = 2;
    cfg.max_quantum = 8;
    cfg.aging_interval = 4;
    return cfg;
}

void compute_predicted_bursts(Process *procs, int n, const SimConfig *cfg) {
    for (int i = 0; i < n; i++) {
        Process *p = &procs[i];
        double tau = cfg->initial_guess;
        for (int b = 0; b < p->num_bursts; b++) {
            p->predicted_burst[b] = tau;
            /* exponential averaging: tau(n+1) = alpha*actual(n) + (1-alpha)*tau(n) */
            tau = cfg->alpha * (double)p->cpu_burst[b] + (1.0 - cfg->alpha) * tau;
        }
    }
}

int input_processes_manual(Process *procs, int max_n) {
    int n;
    printf("Number of processes (max %d): ", max_n);
    scanf("%d", &n);
    if (n < 1) n = 1;
    if (n > max_n) n = max_n;

    for (int i = 0; i < n; i++) {
        Process *p = &procs[i];
        p->pid = i + 1;
        printf("\n--- Process P%d ---\n", p->pid);
        printf("Arrival time: ");
        scanf("%d", &p->arrival_time);
        printf("Base priority (lower = more important): ");
        scanf("%d", &p->base_priority);
        printf("Number of CPU bursts (max %d): ", MAX_BURSTS);
        scanf("%d", &p->num_bursts);
        if (p->num_bursts < 1) p->num_bursts = 1;
        if (p->num_bursts > MAX_BURSTS) p->num_bursts = MAX_BURSTS;

        for (int b = 0; b < p->num_bursts; b++) {
            printf("  CPU burst #%d length: ", b + 1);
            scanf("%d", &p->cpu_burst[b]);
            if (b < p->num_bursts - 1) {
                printf("  I/O burst after it: ");
                scanf("%d", &p->io_burst[b]);
            } else {
                p->io_burst[b] = 0;
            }
        }
    }
    return n;
}

int generate_processes_random(Process *procs, int n, int seed) {
    if (n > MAX_PROCESSES) n = MAX_PROCESSES;
    srand((unsigned int)seed);

    for (int i = 0; i < n; i++) {
        Process *p = &procs[i];
        p->pid = i + 1;
        p->arrival_time = (i == 0) ? 0 : rand() % (i * 3 + 1);
        p->base_priority = 1 + rand() % 5;
        p->num_bursts = 1 + rand() % 3; /* 1..3 CPU bursts, to showcase prediction */

        for (int b = 0; b < p->num_bursts; b++) {
            p->cpu_burst[b] = 2 + rand() % 9;      /* 2..10 */
            p->io_burst[b] = (b < p->num_bursts - 1) ? (1 + rand() % 6) : 0;
        }
    }
    return n;
}

void print_process_table(const Process *procs, int n) {
    printf("\n%-5s %-8s %-9s %-7s", "PID", "Arrival", "Priority", "Bursts");
    printf(" %-28s %-20s\n", "CPU bursts (actual)", "Predicted (1st burst)");
    for (int i = 0; i < n; i++) {
        const Process *p = &procs[i];
        printf("P%-4d %-8d %-9d %-7d", p->pid, p->arrival_time, p->base_priority, p->num_bursts);
        char buf[128] = {0};
        int off = 0;
        for (int b = 0; b < p->num_bursts; b++) {
            off += snprintf(buf + off, sizeof(buf) - off, "%d%s", p->cpu_burst[b],
                             (b < p->num_bursts - 1) ? "," : "");
        }
        printf(" %-28s %-20.2f\n", buf, p->predicted_burst[0]);
    }
}
