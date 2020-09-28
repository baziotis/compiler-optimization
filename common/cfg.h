#ifndef CFG_H
#define CFG_H

#include <new>
#include <stdio.h>

#include "buf.h"
#include "list.h"
#include "stefanos.h"

//   Note that with such few kinds, you could use one kind for all, e.g.
//   INST_DEF_ADD, INST_DEF_SIMPLE, INST_PRINT_ADD, ...

// TODO: Enforce that the entry block has no predecessors and that the last
// block has no successors

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

enum class INST {
  DEF,
  PRINT,
  BR_COND,
  BR_UNCOND,
};

struct BasicBlock;

struct Instruction : ListNode<Instruction, BasicBlock> {
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

  static Instruction *def(uint32_t reg, Operation op) {
    Instruction *i = new Instruction;
    i->kind = INST::DEF;
    i->reg = reg;
    i->op = op;
    return i;
  }

  static Instruction *print(Operation op) {
    Instruction *i = new Instruction;
    i->kind = INST::PRINT;
    i->op = op;
    return i;
  }

  static Instruction *br_uncond(int lbl) {
    Instruction *i = new Instruction;
    i->kind = INST::BR_UNCOND;
    i->uncond_lbl = lbl;
    return i;
  }

  static Instruction *br_cond(Value val, int lbl1, int lbl2) {
    Instruction *i = new Instruction;
    i->kind = INST::BR_COND;
    i->cond_val = val;
    i->then = lbl1;
    i->els = lbl2;
    return i;
  }
};

struct BasicBlock {
  int num;
  Buf<int> succs;
  Buf<int> preds;
  using InstListTy = ListWithParent<Instruction, BasicBlock>;
  InstListTy insts;

  InstListTy *get_sublist() {
    return &insts;
  }

  BasicBlock() {}

  bool has_successor(int bb_id) const {
    for (int succ : succs) {
      if (succ == bb_id)
        return true;
    }
    return false;
  }

  void insert_inst_at_end(Instruction *inst) {
    insts.insert_at_end(inst);
  }
};

// TODO: Since basic blocks are identified by ID, which is
// an integer, it might be good to make a custom type, like
// BasicBlockID or sth. and just use `int`.
struct CFG {
  Buf<BasicBlock> bbs;

  CFG(size_t nbbs) {
    bbs.reserve_and_set(nbbs);
    bbs.initialize();
    LOOP(i, 0, bbs.len()) {
      bbs[i].num = i;
    }
  }

  void destruct() {
    for (BasicBlock &bb : bbs) {
      bb.preds.free();
      bb.succs.free();
    }
    bbs.free();
  }

  // Add edges b -> [succs], where b is indexed basic block
  // in cfg.bbs. Set both succs and preds.
  void add_edges(int b, Buf<int> succs) {
    assert(b < this->size());
    BasicBlock *bb = &(this->bbs[b]);
    assert(bb->succs.len() == 0);
    bb->succs.reserve(succs.len());
    for (int succ : succs) {
      assert(succ < this->size());
      bb->succs.push(succ);
      this->bbs[succ].preds.push(b);
    }
  }

  void add_edge(int source, int dest) {
    assert(source < this->size());
    assert(dest < this->size());
    this->bbs[source].succs.push(dest);
    this->bbs[dest].preds.push(source);
  }

  ssize_t size() const {
    return bbs.len();
  }
};

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
VAL val_kind(Value v) { return (VAL)(v >> 31); }

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
  Operation o = {.kind = OP_SIMPLE, .lhs = lhs};
  return o;
}

static
Operation op_add(Value lhs, Value rhs) {
  Operation o = {.kind = OP_ADD, .lhs = lhs, .rhs = rhs};
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
void inst_print_out(Instruction i) {
  printf("  ");
  switch (i.kind) {
  case INST::DEF:
    printf("%%%u <- ", i.reg);
    break;
  case INST::PRINT:
    printf("PRINT ");
    break;
  case INST::BR_UNCOND:
    printf("BR .%d\t\t", i.uncond_lbl);
    return;
  case INST::BR_COND:
    printf("BR ");
    val_print(i.cond_val);
    printf(", .%d, .%d\t", i.then, i.els);
    return;
  default:
    assert(0);
  }
  op_print(i.op);
}

static
void bb_print(BasicBlock bb) {
  printf("-----------------\n");
  printf(".%d:         ;; ", bb.num);
  printf("preds: ");
  if (bb.preds.len()) {
    printf("%d", bb.preds[0]);
    LOOP(i, 1, bb.preds.len()) {
      printf(", %d", bb.preds[i]);
    }
  }
  printf("    succs: ");
  if (bb.succs.len()) {
    printf("%d", bb.succs[0]);
    LOOP(i, 1, bb.succs.len()) {
      printf(", %d", bb.succs[i]);
    }
  }
  printf("\n");
  for (Instruction *inst : bb.insts) {
    inst_print_out(*inst);
    printf("\n");
  }
  printf("-----------------\n");
}

///* Special CFG constructors */
//
//// Linear cfg i.e. BB_n -> BB_{n+1}
//static
//CFG linear_cfg(int nelems) {
//  CFG cfg;
//  cfg_construct_reserve(&cfg, nelems);
//  LOOP(i, 0, nelems - 1) { cfg_add_edge(cfg, i, i + 1); }
//  return cfg;
//}
//
//// Alternate between:
//// - BB_n -> BB_{n+1}
//// - BB_n -> [ BB_{n+1}, BB_{n-1} ]
//static
//CFG fwdback_cfg(int nelems) {
//  CFG cfg;
//  cfg_construct_reserve(&cfg, nelems);
//  LOOP(i, 0, nelems - 1) {
//    if (i % 2 == 0) {
//      cfg_add_edge(cfg, i, i + 1);
//    } else {
//      cfg_add_edge(cfg, i, i + 1);
//      cfg_add_edge(cfg, i, i - 1);
//    }
//  }
//  return cfg;
//}
//
//// genManyPred creates an array of blocks where 1/3rd have a successor of the
//// first block, 1/3rd the last block, and the remaining third are plain.
//static
//CFG manypred_cfg(int nelems) {
//  CFG cfg;
//  cfg_construct_reserve(&cfg, nelems);
//  LOOP(i, 0, nelems - 1) {
//    switch (i % 3) {
//    case 0:
//      cfg_add_edge(cfg, i, i + 1);
//      break;
//    case 1:
//      cfg_add_edge(cfg, i, i + 1);
//      cfg_add_edge(cfg, i, 0);
//      break;
//    case 2:
//      cfg_add_edge(cfg, i, i + 1);
//      cfg_add_edge(cfg, i, nelems - 1);
//      break;
//    default:
//      assert(0);
//    }
//  }
//  return cfg;
//}

#endif
