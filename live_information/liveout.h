#ifndef LIVEOUT_H
#define LIVEOUT_H

#include "../common/stefanos.h"
#include "../common/bitset.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"
#include "../common/utils.h"

typedef struct LiveInitialInfo {
  Buf<BitSet> UEVar;
  Buf<BitSet> VarKill;
} LiveInitialInfo;

static
void add_if_not_in_VarKill(Value v, BitSet UEVar, BitSet VarKill) {
  if (val_kind(v) == VAL_REG) {
    Value strip = val_strip_kind(v);
    if(!bset_is_in(VarKill, strip)) {
      bset_add(UEVar, strip);
    }
  }
}

static
void print_bitset(BitSet s) {
  for (int i = 0; i < s.max_elems; ++i) {
    if (bset_is_in(s, i))
      printf("%d ", i);
  }
  printf("\n");
}

// Assume bitsets are allocated and initialized to 0
static
void gather_info_for_block(const BasicBlock bb, BitSet UEVar, BitSet VarKill) {
  for (Instruction *i : bb.insts) {
    switch (i->kind) {
    case INST::DEF:
    {
      Operation op = i->op;
      Value lhs = op.lhs;
      Value rhs = op.rhs;
      add_if_not_in_VarKill(lhs, UEVar, VarKill);
      if (op.kind != OP_SIMPLE) {
        add_if_not_in_VarKill(rhs, UEVar, VarKill);
      }
      bset_add(VarKill, val_strip_kind(i->reg));
    } break;
    case INST::PRINT:
    {
      add_if_not_in_VarKill(i->op.lhs, UEVar, VarKill);
    } break;
    case INST::BR_COND:
    {
      add_if_not_in_VarKill(i->cond_val, UEVar, VarKill);
    } break;
    case INST::BR_UNCOND:
      // Nothing
      break;
    default:
      assert(0);
    }
  }
}

static
LiveInitialInfo liveout_gather_initial_info(CFG cfg) {
  LiveInitialInfo res;
  uint32_t nbbs = cfg.size();
  res.UEVar.reserve_and_set(nbbs);
  res.VarKill.reserve_and_set(nbbs);

  size_t base_size = sizeof(BitSet64) * num_words(nbbs);
  uint8_t *mem = (uint8_t *) calloc(base_size, 2*nbbs);
  uint8_t *p1 = mem;
  uint8_t *p2 = mem + base_size*nbbs;
  LOOPu32(i, 0, nbbs) {
    res.UEVar[i] = bset_mem(nbbs, p1);
    res.VarKill[i] = bset_mem(nbbs, p2);
    p1 += base_size;
    p2 += base_size;
  }

  int i = 0;
  for (BasicBlock bb : cfg.bbs) {
    printf("-----------------\n");
    bb.print();
    printf("-----------------\n");
    printf("\n");
    gather_info_for_block(bb, res.UEVar[i], res.VarKill[i]);
    printf("\tUEVar: ");
    print_bitset(res.UEVar[i]);
    printf("\tVarKill: ");
    print_bitset(res.VarKill[i]);
    printf("\n");
    ++i;
  }
  return res;
}

static
void liveout_free_initial_info(LiveInitialInfo init_info) {
  bset_free(init_info.UEVar[0]);
  init_info.UEVar.free();
  init_info.VarKill.free();
}

static
void liveout_solve_equ_for_bb(Buf<BitSet> LiveOut, LiveInitialInfo init_info,
                              BitSet temp, uint32_t bb_num, const Buf<int> succs) {
  BitSet liveout_for_bb = LiveOut[bb_num];
  for (int succ : succs) {
    bset_copy(temp, init_info.VarKill[succ]);
    bset_not(temp);
    intersect_equal_sets_in_place(temp, LiveOut[succ]);
    union_equal_sets_in_place(temp, init_info.UEVar[succ]);
    union_equal_sets_in_place(liveout_for_bb, temp);
  }
}

static
Buf<BitSet> liveout_info(CFG cfg, int max_register) {
  // Get initial info
  LiveInitialInfo init_info = liveout_gather_initial_info(cfg);

  // Get postorder
  Buf<int> postorder = postorder_dfs(cfg);

  // Allocate memory for the bitsets
  int num_registers = max_register + 1;
  int nbbs = cfg.size();

  size_t base_size = sizeof(BitSet64) * num_words(num_registers);
  // +2 for the temps
  uint8_t *mem = (uint8_t *) calloc(base_size, nbbs + 2);
  Buf<BitSet> LiveOut;
  LiveOut.reserve_and_set(nbbs + 2);
  LOOP(i, 0, nbbs) {
    LiveOut[i] = bset_mem(num_registers, mem);
    mem += base_size;
  }
  BitSet temp1 = bset_mem(num_registers, mem);
  BitSet temp2 = bset_mem(num_registers, mem + base_size);

  // Main fixed-point loop.
  int changed = 0;
  int iteration = 1;
  do {
    changed = 0;
    for (int i : postorder) {
      BasicBlock bb = cfg.bbs[i];
      bset_copy(temp1, LiveOut[i]);
      liveout_solve_equ_for_bb(LiveOut, init_info, temp2, i, bb.succs);
      if (!bset_eq(temp1, LiveOut[i])) {
        changed = 1;
      }
    }
    printf("After iteration %d\n", iteration);
    LOOPu32(i, 0, cfg.size()) {
      printf("BB%u: ", i);
      print_bitset(LiveOut[i]);
    }
    ++iteration;
  } while (changed);

  liveout_free_initial_info(init_info);
  postorder.free();

  return LiveOut;
}

static
void liveout_free(Buf<BitSet> LiveOut) {
  bset_free(LiveOut[0]);
  LiveOut.free();
}

#endif
