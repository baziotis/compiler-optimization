#include <stdio.h>

#include "../common/bitset.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "dtree.h"
#include "dom_frontiers.h"

void print_bitset(BitSet bset) {
  LOOPu32(i, 0, bset.max_elems) {
    if (bset_is_in(bset, i)) {
      printf("%d ", i);
    }
  }
}

void print_dom_fronts(DominanceFrontiers dom_fronts) {
  LOOPu32(i, 0, buf_len(dom_fronts.DF)) {
    printf("%d: ", i);
    print_bitset(dom_fronts.DF[i]);
    printf("\n");
  }
}

int main(int argc, char **argv) {
  assert(argc == 2);
  CFG cfg = parse_procedure(argv[1], NULL);
  DominatorTree dtree = dtree_build(cfg);

  printf("\n-- Dominators --\n");
  print_dominators(cfg, dtree);

  DominanceFrontiers dfronts = dom_frontiers(cfg, dtree);
  printf("\n\n-- Dominance Frontiers --\n");
  print_dom_fronts(dfronts);

  dom_frontiers_free(dfronts);
  dtree_free(dtree);
  cfg_destruct(&cfg);
}
