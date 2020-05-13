#include "stefanos.h"
#include <string.h>

#define MAX_ELEM 64

typedef uint64_t BitSet64;

static
BitSet64 bset64() {
  return 0;
}

static
BitSet64 bset64_add(BitSet64 bset, int elem) {
  assert(elem < MAX_ELEM);
  bset |= (1UL << elem);
  return bset;
}

static
int bset64_is_in(BitSet64 bset, int elem) {
  assert(elem < MAX_ELEM);
  // This right shift is actually needed otherwise
  // you may get wrong results.
  return (bset & (1UL << elem)) >> elem;
}

static
int bset64_eq(BitSet64 bset1, BitSet64 bset2) {
  return (bset1 == bset2);
}

static
int bset64_not(BitSet64 bset) {
  return ~bset;
}

// We know the underlying sets, i.e. BitSet64, are 64-bit.
#define WORD_SIZE 64

static
uint32_t num_words(uint32_t sz) {
  // i.e. ceil
  return (sz + WORD_SIZE-1) / WORD_SIZE;
}

typedef struct BitSet {
  int max_elems;
  BitSet64* data;
} BitSet;

static
BitSet bset(int max_elems) {
  BitSet64 *mem = (BitSet64 *) calloc(sizeof(BitSet64), num_words(max_elems));
  BitSet bs = { .max_elems = max_elems, .data = mem };
  return bs;
}

static
BitSet bset_mem(int max_elems, void *mem) {
  BitSet bs = { .max_elems = max_elems, .data = (BitSet64 *) mem };
  return bs;
}

typedef struct SubBitset {
  int index;
  int elem;
} SubBitset;

static
SubBitset compute_sub_bitset(int elem) {
  SubBitset result;
  int index = elem / 64;
  result.index = index;
  result.elem = elem - 64*index;
  return result;
}

// Unconditionally add element (i.e.
// no matter if it is in the set or not).
// Return if the set changed.
static
void bset_add(BitSet bset, int elem) {
  assert(elem < bset.max_elems);
  SubBitset sub = compute_sub_bitset(elem);
  bset.data[sub.index] = bset64_add(bset.data[sub.index], sub.elem);
}

static
int bset_is_in(BitSet bset, int elem) {
  assert(elem < bset.max_elems);
  SubBitset sub = compute_sub_bitset(elem);
  return bset64_is_in(bset.data[sub.index], sub.elem);
}

// Copy `b` into `a` set only if the have the same number
// of words. Note that this is a copy and not a duplication,
// i.e. it is assumed that the `a` already
// has allocated memory that is not supposed to change (e.g.
// increase).
// Return if it was successful or not.
static
int bset_copy(BitSet a, BitSet b) {
  uint32_t words_in_b = num_words(b.max_elems);
  if (num_words(a.max_elems) != words_in_b)
    return 1;
  memcpy(a.data, b.data, words_in_b * sizeof(BitSet64));
  return 1;
}

static
void bset_free(BitSet bset) {
  free(bset.data);
}

static
void light_all(BitSet bset) {
  memset(bset.data, 0xff, num_words(bset.max_elems) * sizeof(BitSet64));
}

static
void bset_not(BitSet set) {
  uint32_t nwords = num_words(set.max_elems);
  LOOPu32(i, 0, nwords) {
    set.data[i] = bset64_not(set.data[i]);
  }
}

static
int bset_eq(BitSet a, BitSet b) {
  uint32_t words_in_a = num_words(a.max_elems);
  if (words_in_a != num_words(b.max_elems))
    return 0;
  
  LOOPu32(i, 0, words_in_a) {
    if (a.data[i] != b.data[i])
      return 0;
  }
  return 1;
}

static
BitSet64 intersect_bitsets64(BitSet64 a, BitSet64 b) {
  return a & b;
}

// We expect this loop to be vectorized
static
void intersect_equal_sets_in_place(BitSet a, BitSet b) {
  uint32_t nwords = num_words(a.max_elems);
  LOOPu32(i, 0, nwords) {
    a.data[i] = intersect_bitsets64(a.data[i], b.data[i]);
  }
}

static
BitSet64 union_bitsets64(BitSet64 a, BitSet64 b) {
  return a | b;
}

// We expect this loop to be vectorized
static
void union_equal_sets_in_place(BitSet a, BitSet b) {
  uint32_t nwords = num_words(a.max_elems);
  LOOPu32(i, 0, nwords) {
    a.data[i] = union_bitsets64(a.data[i], b.data[i]);
  }
}
