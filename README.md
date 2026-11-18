# CPU Scheduling Visualizer (with realistic innovations)

A C simulator that models a process as a sequence of **CPU bursts separated by
I/O bursts** (not just one flat number), and runs it through four scheduling
policies on a single tick-based engine, producing a Gantt chart and the usual
metrics (waiting, turnaround, response time, CPU utilization, throughput,
context switches) for each.

## Build

Needs a C compiler. This machine has no `gcc` on Windows PATH, but **WSL
(Ubuntu) already has gcc 13** installed, so build from there:

```bash
wsl
cd "/mnt/d/Academics Semester 5/14 - CSL301 - Operating Systems/CPU-Scheduler-Project"
make
./scheduler
```

(Alternatively, install MinGW-w64/TDM-GCC on Windows and `mingw32-make` will
work the same way with the same `Makefile`.)

## Why this isn't "just another textbook scheduler demo"

Most student implementations of these four algorithms quietly assume facts a
real OS scheduler never has. This project removes those assumptions:

### 1. SJF uses a *predicted* burst time, not the real one
A real scheduler can't see the future. This simulator predicts the length of
a process's next CPU burst using **exponential averaging** (Silberschatz,
*Operating System Concepts*, "Predicting the Length of the Next CPU Burst"):

```
tau(n+1) = alpha * actual_burst(n) + (1 - alpha) * tau(n)
```

Each process has 1–3 CPU bursts (separated by I/O), and the predicted value
for burst *n* is computed **before** that burst is known — using only the
process's own history. `src/process.c::compute_predicted_bursts` builds this
table once per workload; `engine.c` (`POLICY_SJF_PREDICTED`) schedules purely
on `predicted_burst[]`, never peeking at the real value. Menu option 3 prints
actual vs. predicted side by side so you can show the prediction error.

**Viva answer:** "Our SJF doesn't cheat — it estimates burst time the same
way Silberschatz describes real schedulers doing it, so its schedule can
differ from a 'perfect-knowledge' SJF, which is realistic and demonstrable."

### 2. Round Robin's time quantum is adaptive, not fixed
Instead of one constant quantum for the whole run, the quantum is
recomputed **every time the CPU is handed out**, as the average remaining
burst time of everyone currently in the ready queue, clamped to
`[min_quantum, max_quantum]` (`engine.c`, `POLICY_RR_ADAPTIVE`). A queue of
short jobs gets a short quantum (less turnaround penalty); a queue of long
jobs gets a longer one (fewer wasted context switches).

**Viva answer:** "A fixed quantum is a guess that's wrong for some workload
mixes. We recompute it from the live ready queue so RR adapts to whatever is
actually waiting."

### 3. Priority scheduling ages processes to prevent starvation
Every process accumulates a `wait_timer`; once it has waited
`aging_interval` ticks without running, its numeric priority improves by 1
(`engine.c`, `POLICY_PRIORITY_AGING`). This is the standard textbook fix for
priority-scheduling starvation, actually implemented and visible in the
Gantt chart (a low-priority process does eventually run).

### Bonus: side-by-side comparison dashboard
Menu option 6 runs the **same** workload through all four policies and
prints one table of avg waiting/turnaround/response time, context switches,
and CPU utilization — so you can show tradeoffs instead of one number.

## Project layout

```
include/process.h   Process model + prediction config
include/engine.h     Policy enum, SimResult, run_simulation()
include/gantt.h      Printing helpers
src/process.c        Manual/random process generation + burst prediction
src/engine.c         Tick-based simulation core (all 4 policies)
src/gantt.c          Gantt chart + metrics + comparison table printing
src/main.c           Menu-driven CLI
```

## Using it

1. `2` → generate a random workload (or `1` for manual entry).
2. `3` → see actual vs. predicted first-burst times.
3. `4` → tune alpha / initial guess / quantum bounds / aging interval.
4. `5` → run one algorithm and see its Gantt chart + metrics.
5. `6` → compare all four algorithms on the same workload.
