#ifndef DOMTREE_H
#define DOMTREE_H

#include "../common/bitset.h"
#include "../common/buf.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/stefanos.h"
#include "../common/utils.h"

#define UNDEFINED_IDOM -1

/*
The DominatorTree contains the `idoms` array, which maps
any basic block to its immediate dominator.
Any dominance information is either explicitly or implicitly
obtained using the `idoms` array of the CFG. That is:
- To get an immediate dominator of a node, you just index the array
- To get all the dominators, you just loop the immediate dominators
  until you get to the entry.
- The dominator tree is implicit but also limited
  because we can only walk from leaves to the entry. Generally
  this is no problem because it let's us answer the question
  "Does `a` dominate `b`" -> Start from `b` and go up. If you see
  `a` then yes, otherwise no.

We use the Cooper, Harvey, Kennedy Algorithm
*/

typedef struct DominatorTree {
  int *idoms;
} DominatorTree;

// This function is meant only for internal use. Use dtree_construct()
// to get a DominatorTree
static
DominatorTree dtree_allocate(int number_bbs) {
  // Assert that the user must have accounted for entry and exit.
  assert(number_bbs >= 2);
  DominatorTree dtree = { .idoms = NULL };
  buf_reserve_and_set(dtree.idoms, number_bbs);

  dtree.idoms[0] = 0;
  LOOPu32(i, 1, number_bbs) {
    dtree.idoms[i] = UNDEFINED_IDOM;
  }
  return dtree;
}

static 
int intersect(int b1, int b2, const int *idoms, const int *postorder_map) {
  while (b1 != b2) {
    if (postorder_map[b1] < postorder_map[b2]) {
      b1 = idoms[b1];
    } else {
      b2 = idoms[b2];
    }
  }
  return b1;
}

static
DominatorTree dtree_build(CFG cfg) {
  uint32_t cfg_size = buf_len(cfg.bbs);
  DominatorTree dtree = dtree_allocate(cfg_size);
  int *postorder = postorder_dfs(cfg);
  int *postorder_map = NULL;
  buf_reserve_and_set(postorder_map, cfg_size);
  postorder_map[0] = cfg_size - 1;
  LOOPu32(i, 0, buf_len(postorder)) {
    postorder_map[postorder[i]] = i;
  }

  assert(buf_len(postorder) >= 1);
  int change = 0;
  do {
    change = 0;
    // -1 because we don't want to visit
    // the entry block, which is always 0.
    LOOP_REV(i, 0, buf_len(postorder) - 1) {
      int bb_num = postorder[i];
      BasicBlock bb = cfg.bbs[bb_num];
      int new_idom = bb.preds[0];
      LOOP(j, 1, buf_len(bb.preds)) {
        int pred = bb.preds[j];
        if (dtree.idoms[pred] != UNDEFINED_IDOM) {
          new_idom = intersect(new_idom, pred, dtree.idoms, postorder_map);
        }
      }
      if (dtree.idoms[bb_num] != new_idom) {
        dtree.idoms[bb_num] = new_idom;
        change = 1;
      }
    }
  } while (change);

  buf_free(postorder_map);
  buf_free(postorder);

  return dtree;
}

// Return the immediate dominator of `bb`
int dtree_idom(DominatorTree dtree, int bb) {
  return dtree.idoms[bb];
}

void dtree_free(DominatorTree dtree) {
  buf_free(dtree.idoms);
}

int dtree_num_nodes(DominatorTree dtree) {
  return buf_len(dtree.idoms);
}

// Arbitrary useful routines that are meant for debug purposes

static
void loop_and_print_dominators(DominatorTree dtree, int bb) {
  int idom = bb;
  printf("%d", idom);
  if (idom == 0) {
    printf("\n");
    return;
  }
  do {
    idom = dtree.idoms[idom];
    printf(" %d", idom);
  } while (idom != 0);
  printf("\n");
}

static
void print_dominators(CFG cfg, DominatorTree dtree) {
  LOOP(i, 0, buf_len(cfg.bbs)) {
    printf("%d: ", i);
    loop_and_print_dominators(dtree, i);
  }
}

#undef UNDEFINED_IDOM


/* Benchmark utilities */

static
void dtree_benchmark_comp_and_count(CFG cfg, int nelems) {
  double time_taken;
  TIME_STMT(dtree_build(cfg), time_taken);
  printf("Benchmark CHK: %d elements: %.4lfs\n", nelems, time_taken);
}

static
void dtree_benchmark_linear(int nelems) {
  CFG cfg = linear_cfg(nelems);
  dtree_benchmark_comp_and_count(cfg, nelems);
  cfg_destruct(&cfg);
}

static
void dtree_benchmark_fwdback(int nelems) {
  CFG cfg = fwdback_cfg(nelems);
  dtree_benchmark_comp_and_count(cfg, nelems);
  cfg_destruct(&cfg);
}

static
void dtree_benchmark_manypred(int nelems) {
  CFG cfg = manypred_cfg(nelems);
  dtree_benchmark_comp_and_count(cfg, nelems);
  cfg_destruct(&cfg);
}

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

#endif
