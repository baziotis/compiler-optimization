#ifndef CFG_H
#define CFG_H

//   Note that with such few kinds, you could use one kind for all, e.g.
//   INST_DEF_ADD, INST_DEF_SIMPLE, INST_PRINT_ADD, ...

typedef enum VAL {
  VAL_IMM,
  VAL_REG,
} VAL;

typedef uint32_t Value;

typedef enum OP {
  OP_SIMPLE,
  OP_ADD,
} OP;

typedef struct Operation {
  OP kind;
  Value lhs, rhs;
} Operation;


typedef enum INST {
  INST_DEF,
  INST_PRINT,
  INST_BR_COND,
  INST_BR_UNCOND,
} INST;

typedef struct Instruction {
  INST kind;
  union {
    uint32_t reg;
    int uncond_lbl;
    Value cond_val;
  };
  union {
    Operation op;
    struct {
      int then;
      int els;
    };
  };
} Instruction;

typedef struct BasicBlock {
  int *succs;
  int *preds;
  Instruction *insts;
} BasicBlock;

typedef struct CFG {
  BasicBlock *bbs;
} CFG;

// IMPORTANT: This setup of Value requires to strip it every time
// you use it.

#define MSB (1U << 31)

static
Value val_imm(uint32_t v) {
  assert(v < MSB);
  // Return it as it is, i.e. the MSB is unset and thus
  // the value is considered VAL_IMM.
  return v;
}

static
Value val_reg(uint32_t r) {
  assert(r < MSB);
  return (r | MSB);
}

static
VAL val_kind(Value v) {
  return (VAL) (v >> 31);
}

static
Value val_strip_kind(Value v) {
  // Strip the MSB
  return v & (~(1U << 31));
}

static
void val_print(Value v) {
  Value strip = val_strip_kind(v);
  if (val_kind(v) == VAL_REG) {
    printf("%%%u", strip);
  } else {
    printf("%u", strip);
  }
}

static
Operation op_simple(Value lhs) {
  Operation o = { .kind = OP_SIMPLE, .lhs = lhs };
  return o;
}

static
Operation op_add(Value lhs, Value rhs) {
  Operation o = { .kind = OP_ADD, .lhs = lhs, .rhs = rhs };
  return o;
}

static
void op_print(Operation op) {
  val_print(op.lhs);
  if (op.kind == OP_ADD) {
    printf(" + ");
    val_print(op.rhs);
  }
}

static
Instruction inst_def(uint32_t reg, Operation op) {
  Instruction i = { .kind = INST_DEF, .reg = reg, .op = op };
  return i;
}

static
Instruction inst_print(Operation op) {
  assert(op.kind == OP_SIMPLE);
  Instruction i = { .kind = INST_PRINT, .op = op };
  return i;
}

static
Instruction inst_br_uncond(int lbl) {
  Instruction i = { .kind = INST_BR_UNCOND, .uncond_lbl = lbl };
  return i;
}

static
Instruction inst_br_cond(Value val, int lbl1, int lbl2) {
  Instruction i = { .kind = INST_BR_COND, .cond_val = val, .then = lbl1, .els = lbl2 };
  return i;
}

static
void inst_print_out(Instruction i) {
  printf("| ");
  switch (i.kind) {
  case INST_DEF:
    printf("%%%u <- ", i.reg);
    break;
  case INST_PRINT:
    printf("PRINT ");
    break;
  case INST_BR_UNCOND:
    printf("BR .%d\t\t|", i.uncond_lbl);
    return;
  case INST_BR_COND:
    printf("BR ");
    val_print(i.cond_val);
    printf(", .%d, .%d\t|", i.then, i.els);
    return;
  default:
    assert(0);
  }
  op_print(i.op);
  printf("\t|");
}

static
void bb_print(BasicBlock bb) {
  printf("preds: ");
  if (buf_len(bb.preds)) {
    printf("%d", bb.preds[0]);
    LOOPu32(i, 1, buf_len(bb.preds)) {
      printf(", %d", bb.preds[i]);
    }
  }
  printf("    succs: ");
  if (buf_len(bb.succs)) {
    printf("%d", bb.succs[0]);
    LOOPu32(i, 1, buf_len(bb.succs)) {
      printf(", %d", bb.succs[i]);
    }
  }
  printf("\n");
  printf("-----------------\n");
  LOOPu32(i, 0, buf_len(bb.insts)) {
    inst_print_out(bb.insts[i]);
    printf("\n");
  }
  printf("-----------------\n");
}

static
void bb_initialize(BasicBlock *bb) {
  bb->insts = NULL;
  bb->preds = NULL;
  bb->succs = NULL;
}

static
void cfg_construct(CFG *cfg) {
  cfg->bbs = NULL;
}

static
void cfg_construct_reserve(CFG *cfg, uint32_t nbbs) {
  cfg->bbs = NULL;
  buf_reserve_and_set(cfg->bbs, nbbs);
  BasicBlock *end = buf_end(cfg->bbs);
  for (BasicBlock *it = cfg->bbs; it != end; ++it) {
    bb_initialize(it);
  }
}

static
void cfg_destruct(CFG *cfg) {
  LOOPu32(i, 0, buf_len(cfg->bbs)) {
    BasicBlock bb = cfg->bbs[i];
    if (bb.insts)
      buf_free(bb.insts);
    if (bb.preds)
      buf_free(bb.preds);
    if (bb.succs)
      buf_free(bb.succs);
  }
  buf_free(cfg->bbs);
}

// Add edges b -> [succs], where b is indexed basic block
// in cfg.bbs. Set both succs and preds.
static
void cfg_add_edges(CFG cfg, int b, int *succs, size_t nsuccs) {
  assert(b < buf_len(cfg.bbs));
  assert(cfg.bbs[b].succs == NULL);
  BasicBlock *bb = &(cfg.bbs[b]);
  buf_reserve(bb->succs, nsuccs);
  for (size_t i = 0; i != nsuccs; ++i) {
    int succ = succs[i];
    assert(succ < buf_len(cfg.bbs));
    buf_push(bb->succs, succ);
    buf_push(cfg.bbs[succ].preds, b);
  }
}

static
void cfg_add_edge(CFG cfg, int source, int dest) {
  assert(source < buf_len(cfg.bbs));
  assert(dest < buf_len(cfg.bbs));
  buf_push(cfg.bbs[source].succs, dest);
  buf_push(cfg.bbs[dest].preds, source);
}

#endif
