#include "../common/buf.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/stefanos.h"

#include "lvn.h"

int main(int argc, char **argv) {
  assert(argc == 2);
  CFG cfg = parse_procedure(argv[1], NULL);
  LVN lvn;
  for (BasicBlock &bb : cfg.bbs) {
    lvn.apply(&bb);
    lvn.clear();
  }
  lvn.free();
  cfg.print();
  cfg.destruct();
}
