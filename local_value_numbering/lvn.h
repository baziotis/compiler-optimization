#ifndef LVN_H
#define LVN_H

#if 0
#define DEBUG(block) block
#else
#define DEBUG(block) 
#endif

// TODO: This might seem a really bad implementation as far as data-structures
// are concerned, but actually it's not very bad. Usually basic
// blocks are really small (5-10 instructions), so the buffers we're searching
// are really small (i.e. the whole buffer can fit in a couple of cache lines).
// Now, ideally we would convert them to SOA in which case,
// for most basic blocks, each buffer will fit in a cache line or two at most.

struct LVN {
private:
  struct ValueNumber {
    Value val;
    int num;
  };
  
  struct LVNAdd {
    int lnum, rnum;
  
    bool operator==(LVNAdd add) {
      return (add.lnum == lnum) && (add.rnum == rnum);
    }
  };
  
  struct AddNumber {
    LVNAdd lvn_add;
    int num;
  };


  Buf<ValueNumber> number_for_value;
  Buf<AddNumber> number_for_add;
  int counter = 0;
  
  ValueNumber get_number_for_value_or_create(Value val) {
    for (ValueNumber vnum : number_for_value) {
      if (vnum.val == val)
        return vnum;
    }
    ++counter;
 
    DEBUG( 
      printf("1) %d for: ", counter);
      val_print(val);
      printf("\n");
    )
  
    ValueNumber vnum = { .val = val, .num = counter };
    number_for_value.push(vnum);
    return vnum;
  }
  
  int set_number_for_value_or_create(Value val, int num) {
    for (ValueNumber &vnum : number_for_value) {
      if (vnum.val == val) {
        vnum.num = num;
  
        DEBUG(
          printf("2) %d for: ", num);
          val_print(val);
          printf("\n");
        )
  
        return num;
      }
    }
  
    DEBUG(
      printf("3) %d for: ", num);
      val_print(val);
      printf("\n");
    )
  
    ValueNumber vnum = { .val = val, .num = num };
    number_for_value.push(vnum);
    return num;
  }
  
  Value get_value_for_number(int num) {
    for (ValueNumber vnum : number_for_value) {
      if (vnum.num == num) {
        return vnum.val;
      }
    }
    assert(0);
  }
  
  // Return true if it created, otherwise false.
  bool get_number_for_lvnadd_or_create(LVNAdd add, int *num) {
    for (AddNumber add_num : number_for_add) {
      if (add_num.lvn_add == add) {
        *num = add_num.num;
        return false;
      }
    }
    ++counter;
  
    DEBUG(printf("4) %d for: (%d) + (%d)\n", counter, add.lnum, add.rnum);)
  
    AddNumber add_num = { .lvn_add = add, .num = counter };
    number_for_add.push(add_num);
    *num = counter;
    return true;
  }
  
  // Return true if it created
  bool op_add(Operation op, int *lvn_add_num) {
    assert(op.kind == OP_ADD);
    int lnum = get_number_for_value_or_create(op.lhs).num;
    int rnum = get_number_for_value_or_create(op.rhs).num;
    LVNAdd lvn_add = { .lnum = lnum, .rnum = rnum };
    return get_number_for_lvnadd_or_create(lvn_add, lvn_add_num);
  }
  

public:

  void clear() {
    number_for_value.clear();
    number_for_add.clear();
    counter = 0;
  }

  void apply(BasicBlock *bb) {
    DEBUG(printf("---------------\n");)
    for (Instruction *i : bb->insts) {
      if (i->kind == INST::DEF) {
        if (i->op.kind == OP_ADD) {
          int lvn_add_num;
          bool created = op_add(i->op, &lvn_add_num);
          set_number_for_value_or_create(val_reg(i->reg), lvn_add_num);
          if (!created) {
            DEBUG(printf("%d AGAIN!\n", lvn_add_num);)
            Value copy = get_value_for_number(lvn_add_num);
            i->op = op_simple(copy);
          }
        } else {
          int num = get_number_for_value_or_create(i->op.lhs).num;
          set_number_for_value_or_create(val_reg(i->reg), num);
        }
      }
    }
  }

  void free() {
    number_for_value.free();
    number_for_add.free();
  }
};

#endif
