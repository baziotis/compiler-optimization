#include <stdio.h>
#include "../common/buf.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/stefanos.h"
#include "loop_info.h"

int main(int argc, char **argv) {
  assert(argc == 2);
  CFG cfg = parse_procedure(argv[1], NULL);
  LoopInfo li(cfg);
  li.print();
  cfg.destruct();
}
