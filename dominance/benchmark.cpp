#include "dtree.h"

/* Benchmark utilities */

static
void dtree_benchmark_comp_and_count(CFG cfg, int nelems) {
 double time_taken;
 DominatorTree dtree(cfg.size());
 TIME_STMT(dtree.build(cfg), time_taken);
 printf("Benchmark CHK: %d elements: %.4lfs\n", nelems, time_taken);
}

static
void dtree_benchmark_linear(int nelems) {
 CFG cfg = linear_cfg(nelems);
 dtree_benchmark_comp_and_count(cfg, nelems);
}

static
void dtree_benchmark_fwdback(int nelems) {
 CFG cfg = fwdback_cfg(nelems);
 dtree_benchmark_comp_and_count(cfg, nelems);
}

static
void dtree_benchmark_manypred(int nelems) {
 CFG cfg = manypred_cfg(nelems);
 dtree_benchmark_comp_and_count(cfg, nelems);
}

static
void dtree_benchmark(void) {
 int set[] = { 10, 50, 100, 200, 500, 800, 1000, 1500, 2000, 4000, 8000 };
 printf("--- Linear ---\n");
 LOOP(i, 0, ARR_LEN(set)) {
   dtree_benchmark_linear(set[i]);
 }
 printf("\n");
 printf("--- FwdBack ---\n");
 LOOP(i, 0, ARR_LEN(set)) {
   dtree_benchmark_fwdback(set[i]);
 }
 printf("\n");
 printf("--- ManyPred ---\n");
 LOOP(i, 0, ARR_LEN(set)) {
   dtree_benchmark_manypred(set[i]);
 }
 printf("\n");
}

int main() {
  dtree_benchmark();

  return 0;
}