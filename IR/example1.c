#include <stdio.h>

#include "../stefanos.h"
#include "../bitset.h"
#include "../cfg.h"
#include "../parser_ir.h"
#include "../liveout.h"

// Check example1.ir. This is the same CFG constructed by hand and
// with the BR Instructions omitted.
void example1(void) {
  CFG cfg;

  cfg_construct_reserve(&cfg, 5);

  // B0
  buf_push(cfg.bbs[0].insts, inst_def(0, op_simple(val_imm(1))));
  // B1
  buf_push(cfg.bbs[1].insts, inst_print(op_simple(val_reg(1))));
  // B2
  buf_push(cfg.bbs[2].insts, inst_def(1, op_simple(val_reg(0))));
  // B3
  buf_push(cfg.bbs[3].insts, inst_def(1, op_add(val_reg(1), val_reg(0))));
  buf_push(cfg.bbs[3].insts, inst_def(0, op_add(val_reg(0), val_imm(1))));
  // B4
  buf_push(cfg.bbs[4].insts, inst_print(op_simple(val_reg(1))));

  int B0_succs[] = { 1 };
  cfg_add_edges(cfg, 0, B0_succs, ARR_LEN(B0_succs));
  int B1_succs[] = { 2, 3 };
  cfg_add_edges(cfg, 1, B1_succs, ARR_LEN(B1_succs));
  int B2_succs[] = { 3 };
  cfg_add_edges(cfg, 2, B2_succs, ARR_LEN(B2_succs));
  int B3_succs[] = { 1, 4 };
  cfg_add_edges(cfg, 3, B3_succs, ARR_LEN(B3_succs));

  int max_register = 1;
  liveout_info(cfg, max_register);

  cfg_destruct(&cfg);
}

int main(void) {
  example1();
}
