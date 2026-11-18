#include <stdio.h>
#include "gantt.h"

static void print_cell_label(int pid) {
    if (pid == 0) printf(" IDLE ");
    else printf("  P%-2d ", pid);
}

void print_gantt_chart(const SimResult *res) {
    printf("\nGantt chart [%s]\n", res->policy_name);
    printf(" ");
    for (int i = 0; i < res->gantt_count; i++) {
        printf("|");
        print_cell_label(res->gantt[i].pid);
    }
    printf("|\n0");
    for (int i = 0; i < res->gantt_count; i++) {
        printf("%*d", 7, res->gantt[i].end);
    }
    printf("\n");
}

void print_result_table(const SimResult *res) {
    printf("\n%-5s %-8s %-10s %-11s %-9s %-9s\n",
           "PID", "Arrival", "Completion", "Turnaround", "Waiting", "Response");
    for (int i = 0; i < res->n; i++) {
        const ProcessResult *pr = &res->results[i];
        printf("P%-4d %-8d %-10d %-11d %-9d %-9d\n",
               pr->pid, pr->arrival_time, pr->completion_time,
               pr->turnaround_time, pr->waiting_time, pr->response_time);
    }
    printf("\nAvg waiting time    : %.2f\n", res->avg_waiting_time);
    printf("Avg turnaround time : %.2f\n", res->avg_turnaround_time);
    printf("Avg response time   : %.2f\n", res->avg_response_time);
    printf("CPU utilization     : %.2f%%\n", res->cpu_utilization_pct);
    printf("Throughput          : %.4f processes/tick\n", res->throughput);
    printf("Context switches    : %d\n", res->context_switches);
    printf("Total time (makespan): %d\n", res->total_time);
}

void print_comparison_table(const SimResult *results, int count) {
    printf("\n=== Algorithm comparison (same workload) ===\n");
    printf("%-32s %-10s %-12s %-10s %-8s %-10s\n",
           "Policy", "AvgWait", "AvgTurnaround", "AvgResp", "CtxSw", "CPU-Util%");
    for (int i = 0; i < count; i++) {
        const SimResult *r = &results[i];
        printf("%-32s %-10.2f %-12.2f %-10.2f %-8d %-10.2f\n",
               r->policy_name, r->avg_waiting_time, r->avg_turnaround_time,
               r->avg_response_time, r->context_switches, r->cpu_utilization_pct);
    }
}
