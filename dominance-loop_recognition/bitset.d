import core.stdc.stdio : printf;

// This is a set that saves integer elements in the range [0,64]
// Its underlying structure is a bitvector / 64-bit integer and so
// elements are added and removed by "switching on and off"
// bits.
struct BitSet64 {
@nogc:

  enum max_elem = 64;
  ulong data = 0;

  // Unconditionally add element (i.e.
  // no matter if it is in the set or not).
  // Return if the set changed.
  bool add(int elem) {
    assert(elem < max_elem);
    ulong prev = data;
    data |= 1UL << elem;
    return (prev != data);
  }

  bool is_in(int elem) const {
    assert(elem < max_elem);
    return cast(bool) (data & (1UL << elem));
  }

  //// Operator Overloads

  // This is basically the default behavior but we
  // specify it for disambiguation.
  bool opEquals(const BitSet64 b) const {
    return (data == b.data);
  }
}

// We know the underlying sets, i.e. BitSet64, are 64-bit.
enum WORD_SIZE = 64;
uint num_words(uint sz) @nogc {
  // i.e. ceil
  return (sz + WORD_SIZE-1) / WORD_SIZE;
}

struct BitSet {
@nogc:

  int max_elems;
  // `buf` is used for small size optimization
  BitSet64* data = null;

  this(int _max_elems) {
    import core.stdc.stdlib : calloc;
    max_elems = _max_elems;
    data = cast(BitSet64*) calloc(BitSet64.sizeof, num_words(max_elems));
  }

  this(int _max_elems, void* mem) {
    max_elems = _max_elems;
    data = cast(BitSet64*) mem;
  }

  struct SubBitset {
    int index;
    int elem;
  };

  SubBitset compute_sub_bitset(int elem) const {
    assert(elem < max_elems);
    SubBitset result;
    int index = elem / 64;
    result.index = index;
    result.elem = elem - 64*index;
    return result;
  }

  // Unconditionally add element (i.e.
  // no matter if it is in the set or not).
  // Return if the set changed.
  bool add(int elem) {
    SubBitset sub = compute_sub_bitset(elem);
    bool change = data[sub.index].add(sub.elem);
    return change;
  }

  bool is_in(int elem) const {
    SubBitset sub = compute_sub_bitset(elem);
    return data[sub.index].is_in(sub.elem);
  }

  // Copy `set` into this set only if the have the same number
  // of words. Note that this is a copy and not a duplication,
  // i.e. it is assumed that the current set already
  // has allocated memory that is not supposed to change (e.g.
  // increase).
  bool copy(const BitSet set) {
    if (num_words(max_elems) != num_words(set.max_elems))
      return false;
    import core.stdc.string : memcpy;
    max_elems = set.max_elems;
    memcpy(data, set.data, num_words(max_elems) * BitSet64.sizeof);
    return true;
  }

  void light_all() {
    import core.stdc.string : memset;
    memset(data, 0xff, num_words(max_elems) * BitSet64.sizeof);
  }

  void free() {
    static import core.stdc.stdlib;
    core.stdc.stdlib.free(data);
  }

  bool opEquals(const BitSet b) const {
    uint words_in_a = num_words(max_elems);
    if (words_in_a != num_words(b.max_elems))
      return false;
  
    for (uint i = 0; i < words_in_a; ++i) {
      if (data[i] != b.data[i])
        return false;
    }
    return true;
  }
};

BitSet64 intersect_bitsets64(BitSet64 a, BitSet64 b) @nogc {
  static assert (is(typeof(BitSet64.data) == ulong));
  BitSet64 result;
  result.data = a.data & b.data;
  return result;
}

// We expect this loop to be vectorized
BitSet intersect_equal_sets_in_place(BitSet a, BitSet b) @nogc {
  assert(a.max_elems == b.max_elems);
  uint nwords = num_words(a.max_elems);
  for (uint i = 0; i < nwords; ++i) {
    a.data[i] = intersect_bitsets64(a.data[i], b.data[i]);
  }
  return a;
}
