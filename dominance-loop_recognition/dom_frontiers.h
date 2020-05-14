#ifndef DOM_FRONTIERS_H
#define DOM_FRONTIERS_H

#include <stdlib.h>

#include "../common/buf.h"
#include "../common/bitset.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/stefanos.h"
#include "dtree.h"

typedef struct DominanceFrontiers {
  BitSet *DF;
} DominanceFrontiers;

void dom_frontiers_free(DominanceFrontiers dfronts) {
  bset_free(dfronts.DF[0]);
  buf_free(dfronts.DF);
}

static
int is_join_point(BasicBlock bb) {
  return buf_len(bb.preds) > 1;
}

static
DominanceFrontiers dom_frontiers(CFG cfg, DominatorTree dtree) {
  BitSet *DF = NULL;

  // Allocate memory for the DF sets.

  assert(dtree_num_nodes(dtree) == buf_len(cfg.bbs));
  int nbbs = dtree_num_nodes(dtree);
  buf_reserve_and_set(DF, nbbs);

  size_t base_size = sizeof(BitSet64) * num_words(nbbs);
  void *mem = calloc(base_size, nbbs);
  LOOPu32(i, 0, nbbs) {
    DF[i] = bset_mem(nbbs, mem);
    mem += base_size;
  }

  LOOPu32(n, 0, nbbs) {
    int idom_of_n = dtree_idom(dtree, n);
    BasicBlock bb = cfg.bbs[n];
    if (is_join_point(bb)) {
      LOOPu32(p, 0, buf_len(bb.preds)) {
        int pred = bb.preds[p];
        int runner = pred;
        while (runner != idom_of_n) {
          bset_add(DF[runner], n);
          runner = dtree_idom(dtree, runner);
        }
      }
    }
  }
  
  DominanceFrontiers dfronts = { .DF = DF };
  return dfronts;
}

#endif
