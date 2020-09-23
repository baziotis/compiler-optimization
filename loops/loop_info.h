#ifndef LOOP_INFO
#define LOOP_INFO

#include <stdio.h>
#include "../common/buf.h"
#include "../common/bitset.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/stack_int.h"
#include "../common/stefanos.h"
#include "../dominance/dtree.h"

typedef struct Loop {
  int header_num;
  int latch_num;
  int *bbs;
} Loop;

typedef struct LoopInfo {
  Loop *loops;
} LoopInfo;

// TODO: Eventually we want to use a hash table under the hood
// or at least binary search.
static
int loop_bb_in(Loop l, int bb_num) {
  LOOP(i, 0, buf_len(l.bbs)) {
    if (l.bbs[i] == bb_num)
      return 1;
  }
  return 0;
}

static
Loop loop_construct(CFG cfg, int header_num, int latch_num) {
  Loop l = { .header_num = header_num, .latch_num = latch_num, .bbs = NULL };
  buf_push(l.bbs, header_num);
  StackInt s = stack_construct();

  stack_push(&s, latch_num);
  while (!stack_empty(&s)) {
    int p = stack_pop(&s);
    if (!loop_bb_in(l, p)) {
      buf_push(l.bbs, p);
      int *preds = cfg.bbs[p].preds;
      LOOP(i, 0, buf_len(preds)) {
        stack_push(&s, preds[i]);
      }
    }
  }
  stack_free(&s);
  buf_compact(l.bbs);

  return l;
}

static
void loop_free(Loop *l) {
  buf_free(l->bbs);
}

static
LoopInfo loop_info_construct_with_dtree(CFG cfg, DominatorTree dtree) {
  LoopInfo li = { .loops = NULL };

  int len = buf_len(cfg.bbs);
  LOOP(header_num, 0, len) {
    int *preds = cfg.bbs[header_num].preds;
    LOOP(j, 0, buf_len(preds)) {
      int latch_num = preds[j];
      // TODO: With the current scheme, two loops
      // can have the same header. We probably want to
      // have a unique mapping from header to loop (and vice versa).
      if (dtree_dominates(dtree, header_num, latch_num) &&
          dtree_is_reachable_from_entry(dtree, latch_num)) {
        Loop l = loop_construct(cfg, header_num, latch_num);
        buf_push(li.loops, l);
      }
    }
  }
  return li;
}

static
LoopInfo loop_info_construct(CFG cfg) {
  DominatorTree dtree = dtree_build(cfg);
  LoopInfo li = loop_info_construct_with_dtree(cfg, dtree);
  dtree_free(dtree);
  return li;
}

static
void loop_print(Loop l) {
  printf("Loop: %%%d <- %%%d\n", l.header_num, l.latch_num);
  printf("  ");
  LOOP(i, 0, buf_len(l.bbs)) {
    printf("%%%d ", l.bbs[i]);
  }
  printf("\n");
}

static
void loop_info_print(LoopInfo li) {
  LOOP(i, 0, buf_len(li.loops)) {
    Loop l = li.loops[i];
    loop_print(l);
  }
}

#endif
