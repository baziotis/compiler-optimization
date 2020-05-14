#include <stdio.h>

#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/stefanos.h"
#include "dtree.h"

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

void print_dominators(CFG cfg, DominatorTree dtree) {
  LOOP(i, 0, buf_len(cfg.bbs)) {
    printf("%d: ", i);
    loop_and_print_dominators(dtree, i);
  }
}

int main(int argc, char **argv) {
  assert(argc == 2);
  CFG cfg = parse_procedure(argv[1], NULL);
  DominatorTree dtree = dtree_build(cfg);
  print_dominators(cfg, dtree);

  cfg_destruct(&cfg);
}
