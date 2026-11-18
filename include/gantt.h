#ifndef GANTT_H
#define GANTT_H

#include "engine.h"

void print_gantt_chart(const SimResult *res);
void print_result_table(const SimResult *res);
void print_comparison_table(const SimResult *results, int count);

#endif
