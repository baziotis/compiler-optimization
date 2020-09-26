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
  Buf<BitSet> DF;
} DominanceFrontiers;

void dom_frontiers_free(DominanceFrontiers dfronts) {
  bset_free(dfronts.DF[0]);
  dfronts.DF.free();
}

static
int is_join_point(BasicBlock bb) {
  return bb.preds.len() > 1;
}

static
DominanceFrontiers dom_frontiers(const CFG cfg, const DominatorTree dtree) {

  // Allocate memory for the DF sets.

  int nbbs = dtree.size();
  assert(dtree.size() == cfg.size());
  Buf<BitSet> DF;
  DF.reserve_and_set(nbbs);

  size_t base_size = sizeof(BitSet64) * num_words(nbbs);
  uint8_t *mem = (uint8_t *) calloc(base_size, nbbs);
  LOOPu32(i, 0, nbbs) {
    DF[i] = bset_mem(nbbs, mem);
    mem += base_size;
  }

  LOOPu32(n, 0, nbbs) {
    int idom_of_n = dtree.idom(n);
    BasicBlock bb = cfg.bbs[n];
    if (is_join_point(bb)) {
      LOOPu32(p, 0, bb.preds.len()) {
        int pred = bb.preds[p];
        int runner = pred;
        while (runner != idom_of_n) {
          bset_add(DF[runner], n);
          runner = dtree.idom(runner);
        }
      }
    }
  }
  
  DominanceFrontiers dfronts = { .DF = DF };
  return dfronts;
}

#endif
