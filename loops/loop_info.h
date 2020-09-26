#ifndef LOOP_INFO
#define LOOP_INFO

#include <stdio.h>
#include "../common/buf.h"
#include "../common/bitset.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/stack.h"
#include "../common/stefanos.h"
#include "../dominance/dtree.h"

typedef struct Loop {
  int header_num;
  int latch_num;
  Buf<int> bbs;

  Loop() {
    header_num = latch_num = -1;
  }

  Loop(int _header_num, int _latch_num) :
    header_num(_header_num), latch_num(_latch_num) { }

  Loop(CFG cfg, int _header_num, int _latch_num) :
    Loop(_header_num, _latch_num)
  {
    this->add(header_num);
    Stack<int> s;

    s.push(latch_num);
    while (!s.empty()) {
      int p = s.pop();
      if (!this->contains(p)) {
        this->add(p);
        for (int pred : cfg.bbs[p].preds) {
          s.push(pred);
        }
      }
    }
    s.free();
    this->bbs.compact();
  }

  bool contains(int bb_num) const {
    for (int loop_bb_num : this->bbs) {
      if (loop_bb_num == bb_num)
        return true;
    }
    return false;
  }

  size_t size() const {
    return bbs.len();
  }

  void print() const {
    printf("Loop: %%%d <- %%%d\n", header_num, latch_num);
    printf("  ");
    for (int bb_num : bbs) {
      printf("%%%d ", bb_num);
    }
    printf("\n");
  }

  void add(int bb_num) {
    bbs.push(bb_num);
  }

  void free() {
    bbs.free();
  }
} Loop;

typedef struct LoopInfo {
  Buf<Loop> loops;

  LoopInfo(CFG cfg) {
    DominatorTree dtree(cfg);
    *this = LoopInfo(cfg, dtree);
    dtree.free();
  }

  LoopInfo(CFG cfg, DominatorTree dtree) {
    LOOP(header_num, 0, cfg.size()) {
      Buf<int> preds = cfg.bbs[header_num].preds;
      for (int latch_num : preds) {
        // TODO: With the current scheme, two loops
        // can have the same header. We probably want to
        // have a unique mapping from header to loop (and vice versa).
        if (dtree.dominates(header_num, latch_num) &&
            dtree.is_reachable_from_entry(latch_num)) {
          Loop l(cfg, header_num, latch_num);
          this->loops.push(l);
        }
      }
    }
  }

  void print() const {
    for (Loop l : loops) {
      l.print();
    }
  }
} LoopInfo;


#endif
