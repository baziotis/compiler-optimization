#ifndef DATAFLOW_DOM_H
#define DATAFLOW_DOM_H

#include "../common/stack.h"
#include "../common/bitset.h"

struct AlignedMemory {
  void *ptr;
  size_t offset;
};

template <class T, uint32_t alignment>
AlignedMemory aligned_memory(int nelems) {
  size_t size = sizeof(T) * nelems;
  assert(size > 0);
  void *mem = malloc(size + alignment);
  assert(mem);
  memset(mem, 0, size + alignment);
  // Adjust memory pointer to be aligned
  size_t tmp = ((size_t) mem) & (alignment - 1);
  size_t ofs = (alignment - tmp) & (alignment - 1);
  mem = (void *)(((size_t) mem) + ofs);

  return AlignedMemory{mem, ofs};
}

// TODO: Allocate aligned memory for each.
void initialize_dom_mem(Buf<BitSet> dominators, size_t number_bbs) {
  size_t set_size = sizeof(BitSet64) * num_words(number_bbs);
  void *mem = (void *) calloc(set_size, number_bbs);
  void *runner = mem;

  LOOP(i, 0, number_bbs) {
    dominators[i] = bset_mem(number_bbs, runner);
    runner += set_size;
  }
}

// TODO: Doesn't return or fill something because
// currently the batched allocation of `dominators`
// requires special de-allocation. That's ok, it's
// only for benchmarking. We could change it if we
// want to use it.
void compute_dominators(CFG cfg) {
  size_t number_bbs = cfg.size();
  Buf<BitSet> dominators;
  dominators.reserve_and_set(number_bbs);
  initialize_dom_mem(dominators, number_bbs);
  
  Buf<int> postorder = postorder_dfs(cfg);

  // Initialize all the dominator sets except for the entry block
  // (i.e. 0)
  bset_add(dominators[0], 0);
  LOOP(i, 1, number_bbs) {
    light_all(dominators[i]);
  }

  // Get aligned memory because a lot of memcpy / memset
  // will happen.
  AlignedMemory mem = aligned_memory<BitSet64, 64>(number_bbs);
  BitSet temp = bset_mem(number_bbs, mem.ptr);

  // Assert once that the last element in `postorder` is the entry
  // block. This is important because we want to go in reverse postorder
  // but not include the entry block.
  assert(postorder.len() == number_bbs);
  assert(postorder[number_bbs - 1] == 0);
  bool change;
  do {
    change = false;

    LOOP_REV(i, 0, number_bbs - 1) {
      int bbnum = postorder[i];
      light_all(temp);
      Buf<int> preds = cfg.bbs[bbnum].preds;
      LOOP (j, 0, preds.len()) {
        int pred = preds[j];
        intersect_equal_sets_in_place(temp, dominators[pred]);
      }
      bset_add(temp, i);
      if (!bset_eq(temp, dominators[bbnum])) {
        change = true;
        assert(bset_copy(dominators[bbnum], temp));
      }
    }
  } while (change);

  postorder.free();

  void *revert_offset = mem.ptr - mem.offset;
  free(revert_offset);
  
  // Special de-allocation for `dominators` bitsets.
  free(dominators[0].data);

  dominators.free();
}

#endif
