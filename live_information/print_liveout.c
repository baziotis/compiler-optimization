#include <stdio.h>
#include "../common/stefanos.h"
#include "../common/bitset.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "liveout.h"

int main(int argc, char **argv) {
  assert(argc == 2);
  int max_register;
  CFG cfg = parse_procedure(argv[1], &max_register);
  if (buf_len(cfg.bbs)) {
    BitSet *LiveOut = liveout_info(cfg, max_register);
    liveout_free(LiveOut);
  }

  cfg_destruct(&cfg);
}
