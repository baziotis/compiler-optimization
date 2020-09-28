#ifndef DOMTREE_H
#define DOMTREE_H

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

struct DominatorTree {

  DominatorTree(size_t number_bbs) {
    // Assert that the user must have accounted for entry and exit.
    assert(number_bbs >= 2);
    idoms.reserve_and_set(number_bbs);
  }

  DominatorTree(CFG cfg) : DominatorTree(cfg.size()) {
    this->build(cfg);
  }
  
  void initialize() {
    LOOP(i, 0, idoms.len()) {
      idoms[i] = UNDEFINED_IDOM;
    }
  }

  void build(CFG cfg) {
    this->initialize();
    Buf<int> postorder = postorder_dfs(cfg);
    Buf<int> postorder_map;
    postorder_map.reserve_and_set(cfg.size());
    postorder_map[0] = cfg.size() - 1;
    LOOPu32(i, 0, postorder.len()) {
      postorder_map[postorder[i]] = i;
    }

    // The entry block has itself as its immediate dominator.
    idoms[0] = 0;
    int change = 0;
    do {
      change = 0;
      // -1 because we don't want to visit
      // the entry block, which is always 0.
      assert(postorder.len() >= 1);
      LOOP_REV(i, 0, postorder.len() - 1) {
        int bb_num = postorder[i];
        BasicBlock bb = cfg.bbs[bb_num];
        int new_idom = bb.preds[0];
        LOOP(j, 1, bb.preds.len()) {
          int pred = bb.preds[j];
          if (idoms[pred] != UNDEFINED_IDOM) {
            new_idom = intersect(new_idom, pred, idoms, postorder_map);
          }
        }
        if (idoms[bb_num] != new_idom) {
          idoms[bb_num] = new_idom;
          change = 1;
        }
      }
    } while (change);
  }

  // Return the immediate dominator of `bb`
  int idom(int bb) const {
    return idoms[bb];
  }

  // Return true if BB no. `a` dominates BB no. `b`
  bool dominates(int a, int b) const {
    static constexpr int entry_block = 0;
    // If `a` is the entry block, then it dominates any other
    // block (and itself).
    if (a == entry_block) {
      return 1;
    }
    // Start from `b` and go upwards until you either
    // find `a` and `a` dominates `b`, or we reach
    // the entry and we return false (we have already
    // tested that `a` is not the entry).
    int runner = idoms[b];
    while (runner != entry_block) {
      if (runner == a)
        return 1;
      runner = idoms[runner];
    }
    return 0;
  }
  
  bool is_reachable_from_entry(int bb) const {
    return dominates(0, bb);
  }

  ssize_t size() const {
    return idoms.len();
  }

  void free() {
    idoms.free();
  }

private:

  static 
  int intersect(int b1, int b2, const Buf<int> idoms, Buf<int> postorder_map) {
    while (b1 != b2) {
      if (postorder_map[b1] < postorder_map[b2]) {
        b1 = idoms[b1];
      } else {
        b2 = idoms[b2];
      }
    }
    return b1;
  }

  /// Members ///

  Buf<int> idoms;
};


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
    idom = dtree.idom(idom);
    printf(" %d", idom);
  } while (idom != 0);
  printf("\n");
}

static
void print_dominators(CFG cfg, DominatorTree dtree) {
  LOOP(i, 0, cfg.size()) {
    printf("%d: ", i);
    loop_and_print_dominators(dtree, i);
  }
}

#undef UNDEFINED_IDOM


///* Benchmark utilities */
//
//static
//void dtree_benchmark_comp_and_count(CFG cfg, int nelems) {
//  double time_taken;
//  DominatorTree dtree(cfg.size());
//  TIME_STMT(dtree.build(cfg), time_taken);
//  printf("Benchmark CHK: %d elements: %.4lfs\n", nelems, time_taken);
//}
//
//static
//void dtree_benchmark_linear(int nelems) {
//  CFG cfg = linear_cfg(nelems);
//  dtree_benchmark_comp_and_count(cfg, nelems);
//  cfg_destruct(&cfg);
//}
//
//static
//void dtree_benchmark_fwdback(int nelems) {
//  CFG cfg = fwdback_cfg(nelems);
//  dtree_benchmark_comp_and_count(cfg, nelems);
//  cfg_destruct(&cfg);
//}
//
//static
//void dtree_benchmark_manypred(int nelems) {
//  CFG cfg = manypred_cfg(nelems);
//  dtree_benchmark_comp_and_count(cfg, nelems);
//  cfg_destruct(&cfg);
//}
//
//void dtree_benchmark(void) {
//  int set[] = { 10, 50, 100, 200, 500, 800, 1000, 1500, 2000, 4000, 8000 };
//  printf("--- Linear ---\n");
//  LOOP(i, 0, ARR_LEN(set)) {
//    dtree_benchmark_linear(set[i]);
//  }
//  printf("\n");
//  printf("--- FwdBack ---\n");
//  LOOP(i, 0, ARR_LEN(set)) {
//    dtree_benchmark_fwdback(set[i]);
//  }
//  printf("\n");
//  printf("--- ManyPred ---\n");
//  LOOP(i, 0, ARR_LEN(set)) {
//    dtree_benchmark_manypred(set[i]);
//  }
//  printf("\n");
//}

#endif
