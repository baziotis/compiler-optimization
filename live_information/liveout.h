#ifndef LIVEOUT_H
#define LIVEOUT_H

typedef struct LiveInitialInfo {
  BitSet *UEVar;
  BitSet *VarKill;
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
  LOOP(k, 0, buf_len(bb.insts)) {
    Instruction i = bb.insts[k];
    switch (i.kind) {
    case INST_DEF:
    {
      Operation op = i.op;
      Value lhs = op.lhs;
      Value rhs = op.rhs;
      add_if_not_in_VarKill(lhs, UEVar, VarKill);
      if (op.kind != OP_SIMPLE) {
        add_if_not_in_VarKill(rhs, UEVar, VarKill);
      }
      bset_add(VarKill, val_strip_kind(i.reg));
    } break;
    case INST_PRINT:
    {
      add_if_not_in_VarKill(i.op.lhs, UEVar, VarKill);
    } break;
    case INST_BR_COND:
    {
      add_if_not_in_VarKill(i.cond_val, UEVar, VarKill);
    } break;
    case INST_BR_UNCOND:
      // Nothing
      break;
    default:
      assert(0);
    }
  }
}

static
LiveInitialInfo liveout_gather_initial_info(CFG cfg) {
  LiveInitialInfo res = { .UEVar = NULL, .VarKill = NULL };
  uint32_t nbbs = buf_len(cfg.bbs);
  buf_reserve_and_set(res.UEVar, nbbs);
  buf_reserve_and_set(res.VarKill, nbbs);

  size_t base_size = sizeof(BitSet64) * num_words(nbbs);
  void *mem = calloc(base_size, 2*nbbs);
  void *p1 = mem;
  void *p2 = mem + base_size*nbbs;
  LOOPu32(i, 0, nbbs) {
    res.UEVar[i] = bset_mem(nbbs, p1);
    res.VarKill[i] = bset_mem(nbbs, p2);
    p1 += base_size;
    p2 += base_size;
  }

  LOOPu32(i, 0, nbbs) {
    printf("BB: %u\n", i);
    bb_print(cfg.bbs[i]);
    printf("\n");
    gather_info_for_block(cfg.bbs[i], res.UEVar[i], res.VarKill[i]);
    printf("\tUEVar: ");
    print_bitset(res.UEVar[i]);
    printf("\tVarKill: ");
    print_bitset(res.VarKill[i]);
    printf("\n");
  }
  return res;
}

static
void liveout_free_initial_info(LiveInitialInfo init_info) {
  bset_free(init_info.UEVar[0]);
  buf_free(init_info.UEVar);
  buf_free(init_info.VarKill);
}

static
void liveout_solve_equ_for_bb(BitSet *LiveOut, LiveInitialInfo init_info,
                              BitSet temp, uint32_t bb_num, int *succs) {
  uint32_t num_succs = buf_len(succs);
  BitSet liveout_for_bb = LiveOut[bb_num];
  LOOPu32(s, 0, num_succs) {
    int succ = succs[s];
    bset_copy(temp, init_info.VarKill[succ]);
    bset_not(temp);
    intersect_equal_sets_in_place(temp, LiveOut[succ]);
    union_equal_sets_in_place(temp, init_info.UEVar[succ]);
    union_equal_sets_in_place(liveout_for_bb, temp);
  }
}

static
BitSet *liveout_info(CFG cfg, int max_register) {
  LiveInitialInfo init_info = liveout_gather_initial_info(cfg);
  int num_registers = max_register + 1;
  int nbbs = buf_len(cfg.bbs);

  size_t base_size = sizeof(BitSet64) * num_words(num_registers);
  // +2 for the temps
  void *mem = calloc(base_size, nbbs + 2);
  BitSet *LiveOut = NULL;
  buf_reserve_and_set(LiveOut, nbbs + 2);
  LOOP(i, 0, nbbs) {
    LiveOut[i] = bset_mem(num_registers, mem);
    mem += base_size;
  }
  BitSet temp1 = bset_mem(num_registers, mem);
  BitSet temp2 = bset_mem(num_registers, mem + base_size);

  int changed = 0;
  int iteration = 1;
  do {
    changed = 0;
    LOOPu32(i, 0, buf_len(cfg.bbs)) {
      BasicBlock bb = cfg.bbs[i];
      bset_copy(temp1, LiveOut[i]);
      liveout_solve_equ_for_bb(LiveOut, init_info, temp2, i, bb.succs);
      if (!bset_eq(temp1, LiveOut[i])) {
        changed = 1;
      }
    }
    printf("After iteration %d\n", iteration);
    LOOPu32(i, 0, buf_len(cfg.bbs)) {
      printf("BB%u: ", i);
      print_bitset(LiveOut[i]);
    }
    ++iteration;
  } while (changed);

  liveout_free_initial_info(init_info);

  return LiveOut;
}

static
void liveout_free(BitSet *LiveOut) {
  bset_free(LiveOut[0]);
  buf_free(LiveOut);
}

#endif
