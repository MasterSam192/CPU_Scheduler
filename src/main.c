#include <stdio.h>
#include "process.h"
#include "engine.h"
#include "gantt.h"

static Process g_procs[MAX_PROCESSES];
static int g_n = 0;
static SimConfig g_cfg;
static int g_have_processes = 0;

static void refresh_predictions(void) {
    if (g_have_processes) compute_predicted_bursts(g_procs, g_n, &g_cfg);
}

static void menu_manual(void) {
    g_n = input_processes_manual(g_procs, MAX_PROCESSES);
    g_have_processes = 1;
    refresh_predictions();
    print_process_table(g_procs, g_n);
}

static void menu_random(void) {
    int n, seed;
    printf("How many processes? ");
    scanf("%d", &n);
    printf("Random seed (any integer): ");
    scanf("%d", &seed);
    g_n = generate_processes_random(g_procs, n, seed);
    g_have_processes = 1;
    refresh_predictions();
    print_process_table(g_procs, g_n);
}

static void menu_view(void) {
    if (!g_have_processes) { printf("No processes yet.\n"); return; }
    print_process_table(g_procs, g_n);
}

static void menu_configure(void) {
    printf("Current config: alpha=%.2f initial_guess=%.2f min_q=%d max_q=%d aging_interval=%d\n",
           g_cfg.alpha, g_cfg.initial_guess, g_cfg.min_quantum, g_cfg.max_quantum, g_cfg.aging_interval);
    printf("alpha (0-1, weight on most recent burst): ");
    scanf("%lf", &g_cfg.alpha);
    printf("initial burst-time guess: ");
    scanf("%lf", &g_cfg.initial_guess);
    printf("Round Robin min quantum: ");
    scanf("%d", &g_cfg.min_quantum);
    printf("Round Robin max quantum: ");
    scanf("%d", &g_cfg.max_quantum);
    printf("Aging interval (ticks waited before priority improves): ");
    scanf("%d", &g_cfg.aging_interval);
    refresh_predictions();
    printf("Config updated.\n");
}

static void menu_run_one(void) {
    if (!g_have_processes) { printf("No processes yet.\n"); return; }
    printf("Pick algorithm:\n");
    printf(" 1. FCFS\n 2. SJF (predicted burst)\n 3. Round Robin (adaptive quantum)\n 4. Priority (with aging)\n");
    int c;
    scanf("%d", &c);
    Policy pol;
    switch (c) {
        case 1: pol = POLICY_FCFS; break;
        case 2: pol = POLICY_SJF_PREDICTED; break;
        case 3: pol = POLICY_RR_ADAPTIVE; break;
        case 4: pol = POLICY_PRIORITY_AGING; break;
        default: printf("Invalid.\n"); return;
    }
    SimResult res = run_simulation(g_procs, g_n, pol, &g_cfg);
    print_gantt_chart(&res);
    print_result_table(&res);
}

static void menu_compare_all(void) {
    if (!g_have_processes) { printf("No processes yet.\n"); return; }
    SimResult all[4];
    Policy policies[4] = {POLICY_FCFS, POLICY_SJF_PREDICTED, POLICY_RR_ADAPTIVE, POLICY_PRIORITY_AGING};
    for (int i = 0; i < 4; i++) {
        all[i] = run_simulation(g_procs, g_n, policies[i], &g_cfg);
        print_gantt_chart(&all[i]);
    }
    print_comparison_table(all, 4);
}

int main(void) {
    g_cfg = default_config();
    int choice;
    do {
        printf("\n===== CPU Scheduling Visualizer =====\n");
        printf("1. Enter processes manually\n");
        printf("2. Generate random processes\n");
        printf("3. View process table (actual vs predicted bursts)\n");
        printf("4. Configure prediction / quantum / aging parameters\n");
        printf("5. Run one algorithm\n");
        printf("6. Compare all algorithms on this workload\n");
        printf("0. Exit\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) break;
        switch (choice) {
            case 1: menu_manual(); break;
            case 2: menu_random(); break;
            case 3: menu_view(); break;
            case 4: menu_configure(); break;
            case 5: menu_run_one(); break;
            case 6: menu_compare_all(); break;
            case 0: printf("Bye.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
    return 0;
}
