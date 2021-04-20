#include "dtree.h"
#include "dataflow.h"
#include "lengauer-tarjan.h"

/* Benchmark utilities */

static
void dtree_benchmark_comp_and_count(CFG cfg, int nelems) {
 double chk_time_taken, lt_slow_time_taken, dataflow_time_taken;
 DominatorTree dtree(cfg.size());
 Buf<int> idom;
 idom.reserve_and_set(cfg.size());

 TIME_STMT(dtree.build(cfg), chk_time_taken);
 // Slow time is practically no different from fast (i.e., with path compression)
 // for sizes even up to 32000
 TIME_STMT(lt_slow(cfg, idom), lt_slow_time_taken);
 TIME_STMT(compute_dominators(cfg), dataflow_time_taken);

 idom.free();
 dtree.free();

 printf("Benchmark CHK: %d elements: %.4lfs\n", nelems, chk_time_taken);
 printf("Benchmark Lengauer-Tarjan Slow: %d elements: %.4lfs\n", nelems, lt_slow_time_taken);
 printf("Benchmark Dataflow: %d elements: %.4lfs\n", nelems, dataflow_time_taken);
}

static
void dtree_benchmark_linear(int nelems) {
 CFG cfg = linear_cfg(nelems);
 dtree_benchmark_comp_and_count(cfg, nelems);
 cfg.destruct();
}

static
void dtree_benchmark_fwdback(int nelems) {
 CFG cfg = fwdback_cfg(nelems);
 dtree_benchmark_comp_and_count(cfg, nelems);
 cfg.destruct();
}

static
void dtree_benchmark_manypred(int nelems) {
 CFG cfg = manypred_cfg(nelems);
 dtree_benchmark_comp_and_count(cfg, nelems);
 cfg.destruct();
}

static
void dtree_benchmark(void) {
 int set[] = { 10, 50, 100, 200, 500, 800, 1000, 1500, 2000, 4000, 8000, 16000, 32000 };
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
