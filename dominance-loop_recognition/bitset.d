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

  // Unconditionally remove element (i.e.
  // no matter if it is in the set or not).
  // Return if the set changed.
  bool remove(int elem) {
    assert(elem < max_elem);
    ulong prev = data;
    data &= ~(1UL << elem);
    return (prev != data);
  }

  bool is_in(int elem) const {
    assert(elem < max_elem);
    return cast(bool) (data & (1UL << elem));
  }

  ulong bitVector() const {
    return data;
  }

  long size() const {
    import core.bitop : popcnt;
    return popcnt(data);
  }

  //// Operator Overloads

  // This is basically the default behavior but we
  // specify it for disambiguation.
  bool opEquals(const BitSet64 b) const {
    return (size() == b.size() && bitVector() == b.bitVector());
  }

  BitSet64 opBinary(string op)(BitSet64 rhs) const {
    static if (op == "-") {
      BitSet64 result;
      result.data = bitVector() & (~rhs.bitVector());
      return result;
    } else {
      static assert(0, "Operator "~op~" not implemented");
    }
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

  int nelems = 0;
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
    nelems += cast(long) change;
    return change;
  }

  // Unconditionally remove element (i.e.
  // no matter if it is in the set or not).
  // Return if the set changed.
  bool remove(int elem) {
    SubBitset sub = compute_sub_bitset(elem);
    bool change = data[sub.index].remove(sub.elem);
    nelems -= cast(long) change;
    return change;
  }

  bool is_in(int elem) const {
    SubBitset sub = compute_sub_bitset(elem);
    return data[sub.index].is_in(sub.elem);
  }

  long size() const {
    return nelems;
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
    nelems = set.nelems;
    max_elems = set.max_elems;
    memcpy(data, set.data, num_words(max_elems) * BitSet64.sizeof);
    return true;
  }

  void light_all() {
    import core.stdc.string : memset;
    memset(data, 0xff, num_words(max_elems) * BitSet64.sizeof);
    nelems = max_elems;
  }

  void free() {
    static import core.stdc.stdlib;
    core.stdc.stdlib.free(data);
  }

  bool opEquals(const BitSet b) const {
    if (size() != b.size())
      return false;
    
    uint words_in_a = num_words(max_elems);
    if (words_in_a != num_words(b.max_elems))
      return false;
  
    for (uint i = 0; i < words_in_a; ++i) {
      if (data[i] != b.data[i])
        return false;
    }
    return true;
  }

  BitSet opBinary(string op)(BitSet rhs) const {
    static if (op == "-") {
      MinMax minmax = find_minmax(this, rhs);
      BitSet result = BitSet(minmax.min_of_2);
      uint i = 0;
      uint nelems = 0;
      // Subtract only when there are data in both
      for (i = 0; i < minmax.min_nwords; ++i) {
        result.data[i] = diff_bitsets64(data[i], rhs.data[i]);
        nelems += result.data[i].size();
      }
      result.nelems = nelems;
      return result;
    } else {
      static assert(0, "Operator "~op~" not implemented");
    }
  }
};

// TODO: That's wrong! It works only for the union. For the intersection
// actually you want the result to have size of the minimum of the two.

private struct MinMax {
  int max_of_2;
  int min_of_2;
  uint min_nwords;
  uint max_nwords;
  BitSet max_set;
}

private MinMax find_minmax(const BitSet a, const BitSet b) @nogc {
  MinMax result;
  result.max_set = cast(BitSet) a;
  if (a.max_elems > b.max_elems) {
    result.min_of_2 = b.max_elems;
    result.max_of_2 = a.max_elems;
  } else {
    result.max_set = cast(BitSet) b;
    result.max_of_2 = b.max_elems;
    result.min_of_2 = a.max_elems;
  }
  result.min_nwords = num_words(result.min_of_2);
  result.max_nwords = num_words(result.max_of_2);
  return result;
}

BitSet64 union_bitsets64(BitSet64 a, BitSet64 b) @nogc {
  static assert (is(typeof(BitSet64.data) == ulong));
  BitSet64 result;
  result.data = a.bitVector() | b.bitVector();
  return result;
}

BitSet64 intersect_bitsets64(BitSet64 a, BitSet64 b) @nogc {
  static assert (is(typeof(BitSet64.data) == ulong));
  BitSet64 result;
  result.data = a.bitVector() & b.bitVector();
  return result;
}

BitSet64 diff_bitsets64(BitSet64 a, BitSet64 b) @nogc {
  return a - b;
}

T union_sets(T)(T a, T b) @nogc {
  static if (is(T == BitSet64)) {
    return union_bitsets64(a, b);
  } else {
    MinMax minmax = find_minmax(a, b);
    BitSet result = BitSet(minmax.max_of_2);
    uint i = 0;
    uint nelems = 0;
    // Union when there are data in both
    for (i = 0; i < minmax.min_nwords; ++i) {
      result.data[i] = union_bitsets64(a.data[i], b.data[i]);
      nelems += result.data[i].size();
    }
    // Copy exceeding data of the max
    while (i < minmax.max_nwords) {
      result.data[i] = minmax.max_set.data[i];
      nelems += result.data[i].size();
    }
    result.nelems = nelems;
    return result;
  }
}

// Specialized function which comes up a lot of times in practice.
BitSet union_equal_sets_in_place(BitSet a, BitSet b) @nogc {
  assert(a.max_elems == b.max_elems);
  uint nelems = 0;
  uint nwords = num_words(a.max_elems);
  for (uint i = 0; i < nwords; ++i) {
    a.data[i] = union_bitsets64(a.data[i], b.data[i]);
    nelems += a.data[i].size();
  }
  a.nelems = nelems;
  return a;
}

T intersect_sets(T)(T a, T b) @nogc {
  static if (is(T == BitSet64)) {
    return intersect_bitsets64(a, b);
  } else {
    MinMax minmax = find_minmax(a, b);
    BitSet result = BitSet(minmax.min_of_2);
    uint i = 0;
    uint nelems = 0;
    // Intersect only when there are data in both
    for (i = 0; i < minmax.min_nwords; ++i) {
      result.data[i] = intersect_bitsets64(a.data[i], b.data[i]);
      nelems += result.data[i].size();
    }
    result.nelems = nelems;
    return result;
  }
}

// Specialized function which comes up a lot of times in practice.
BitSet intersect_equal_sets_in_place(BitSet a, BitSet b) @nogc {
  assert(a.max_elems == b.max_elems);
  uint nelems = 0;
  uint nwords = num_words(a.max_elems);
  for (uint i = 0; i < nwords; ++i) {
    a.data[i] = intersect_bitsets64(a.data[i], b.data[i]);
    nelems += a.data[i].size();
  }
  a.nelems = nelems;
  return a;
}

import std.random : Random, uniform, randomShuffle;


//////////////////////////// BitSet64 Tests //////////////////////////////////

// Generic tests
unittest {
  // Dumb implementation
  int count_unique(int[] arr) {
    int[int] lookup;
    int count = 0;
    foreach (elem ; arr) {
      int* p;
      p = (elem in lookup);
      if (p is null) {
        lookup[elem] = 1;
        count++;
      }
    }
    return count;
  }

  BitSet64 set;
  for (int i = 0; i < BitSet64.max_elem; ++i)
    assert(!set.is_in(i));

  // Note that two of the elements to be added (0, 4)
  // is added twice ; We shouldn't have any problem.
  int[] to_be_added = [ 0, 63, 43, 0, 9, 2, 7, 7 ];

  foreach (elem ; to_be_added) {
    set.add(elem);
  }
  // We don't want to be equal because the elements
  // are not unique.
  assert(set.size() != to_be_added.length);
  assert(set.size() == count_unique(to_be_added));

  import std.algorithm : canFind;

  for (int i = 0; i < BitSet64.max_elem; ++i) {
    assert(set.is_in(i) == to_be_added.canFind(i));
  }

  // Note that one of the elements to be removed (8)
  // is not in the set ; We shouldn't have any problem.
  int[] to_be_removed = [ 0, 63, 2, 8 ];

  long prev_size = set.size();
  long actually_removed = 0;
  foreach (elem ; to_be_removed) {
    actually_removed += set.remove(elem);
  }

  assert(set.size() == prev_size - actually_removed);

  for (int i = 0; i < BitSet64.max_elem; ++i) {
    if (to_be_added.canFind(i)) {
      if (to_be_removed.canFind(i)) {
        assert(set.is_in(i) == false);
      } else {
        assert(set.is_in(i) == true);
      }
    } else {
      assert(set.is_in(i) == false);
    }
  }
}

// Equality Test
unittest {
  BitSet64 a, b;

  int[] common_els = [ 0, 45, 8 ];
  foreach (elem ; common_els) {
    a.add(elem);
    b.add(elem);
  }

  assert(a == b);
  
  // Same sizes but different contents
  BitSet64 c, d;
  c.add(1);
  c.add(2);

  d.add(1);
  d.add(3);

  assert(c != d);

  // Different sizes

  BitSet64 e, f;
  e.add(1);
  e.add(2);

  f.add(1);
  assert(e != f);
}

// Union tests

unittest {
  BitSet64 a, b;

  // Note that two of the elements (0, 9) are common
  // in the two sets ; Obviously this should be handled.
  // Also note that the two sets are of different sizes.
  int[] elements_of_a = [ 0, 63, 9, 7 ];
  int[] elements_of_b = [ 0, 34, 29, 9, 17, 19 ];

  // Add elements
  foreach (elem ; elements_of_a) {
    a.add(elem);
  }
  foreach (elem ; elements_of_b) {
    b.add(elem);
  }

  import std.algorithm : canFind;

  BitSet64 un1 = union_sets(a, b);
  long correct_size = 0;
  // Find the size pedantically
  for (int i = 0; i < BitSet64.max_elem; ++i) {
    correct_size += elements_of_a.canFind(i) || elements_of_b.canFind(i);
  }
  assert(un1.size() == correct_size);
  // Check the size pedantically
  long sz = 0;
  for (int i = 0; i < BitSet64.max_elem; ++i) {
    sz += un1.is_in(i);
  }
  assert(sz == correct_size);

  // Check the elements
  for (int i = 0; i < BitSet64.max_elem; ++i) {
    if (un1.is_in(i)) {
      assert(elements_of_a.canFind(i) || elements_of_b.canFind(i));
    } else {
      assert(!elements_of_a.canFind(i) && !elements_of_b.canFind(i));
    }
  }

  BitSet64 empty;
  BitSet64 un2 = union_sets(un1, empty);
  assert(un2 == un1);
}

// Intersection tests

unittest {
  BitSet64 a, b;

  // Note that two of the elements (0, 9) are common
  // in the two sets ; Obviously these should be the
  // only elements in the intersection.
  int[] elements_of_a = [ 0, 63, 9, 7 ];
  int[] elements_of_b = [ 0, 34, 29, 9, 17, 19 ];

  // Add elements
  foreach (elem ; elements_of_a) {
    a.add(elem);
  }
  foreach (elem ; elements_of_b) {
    b.add(elem);
  }

  import std.algorithm : canFind;

  BitSet64 inter1 = intersect_sets(a, b);
  long correct_size = 0;
  // Find the size pedantically
  for (int i = 0; i < BitSet64.max_elem; ++i) {
    correct_size += elements_of_a.canFind(i) && elements_of_b.canFind(i);
  }
  assert(inter1.size() == correct_size);
  // Check the size pedantically
  long sz = 0;
  for (int i = 0; i < BitSet64.max_elem; ++i) {
    sz += inter1.is_in(i);
  }
  assert(sz == correct_size);

  // Check the elements
  for (int i = 0; i < BitSet64.max_elem; ++i) {
    if (inter1.is_in(i)) {
      assert(elements_of_a.canFind(i) && elements_of_b.canFind(i));
    }
  }

  BitSet64 empty;
  BitSet64 inter2 = intersect_sets(inter1, empty);
  assert(inter2.size() == 0);
  for (int i = 0; i < BitSet64.max_elem; ++i) {
    assert(!inter2.is_in(i));
  }
}

// Difference tests

unittest {
  BitSet64 a, b;

  // Note that two of the elements (0, 9) are common
  // in the two sets ; a - b should have all elements
  // of `a` except those 2.
  int[] elements_of_a = [ 0, 63, 9, 7 ];
  int[] elements_of_b = [ 0, 34, 29, 9, 17, 19 ];

  // Add elements
  foreach (elem ; elements_of_a) {
    a.add(elem);
  }
  foreach (elem ; elements_of_b) {
    b.add(elem);
  }

  BitSet64 diff1 = a - b;
  assert(diff1.size() == a.size() - 2);

  for (int i = 0; i < BitSet64.max_elem; ++i) {
    if (diff1.is_in(i)) {
      assert(a.is_in(i) && !b.is_in(i));
    }
  }

  BitSet64 empty;
  BitSet64 diff2 = a - empty;
  assert(diff2 == a);
}

//////////////////////////// BitSet Tests //////////////////////////////////

// Generic tests
unittest {
  // Dumb implementation
  int count_unique(int[] arr) {
    int[int] lookup;
    int count = 0;
    foreach (elem ; arr) {
      int* p;
      p = (elem in lookup);
      if (p is null) {
        lookup[elem] = 1;
        count++;
      }
    }
    return count;
  }

  BitSet set = BitSet(256);
  for (int i = 0; i < set.max_elems; ++i)
    assert(!set.is_in(i));

  // seed a random generator
  auto rnd = Random(42);
  
  enum NELEMS = 234;
  int[] to_be_added;
  to_be_added.length = NELEMS;
  // Note that some of them might be duplicates.
  for (int i = 0; i < NELEMS; ++i) {
    to_be_added[i] = uniform(0, NELEMS, rnd);
  }

  foreach (elem ; to_be_added) {
    set.add(elem);
  }

  assert(set.size() == count_unique(to_be_added));

  import std.algorithm : canFind;

  for (int i = 0; i < set.max_elems; ++i) {
    assert(set.is_in(i) == to_be_added.canFind(i));
  }

  // Note that one of the elements to be removed (8)
  // is not in the set ; We shouldn't have any problem.
  enum NREMOVE = 50;
  int[] to_be_removed;
  to_be_removed.length = NREMOVE;
  for (int i = 0; i < NREMOVE; ++i) {
    int elem;
    do {
      elem = uniform(0, NELEMS, rnd);
    } while (set.is_in(elem));
    to_be_removed[i] = elem;
  }

  long prev_size = set.size();
  long actually_removed = 0;
  foreach (elem ; to_be_removed) {
    actually_removed += set.remove(elem);
  }

  assert(set.size() == prev_size - actually_removed);

  for (int i = 0; i < set.max_elems; ++i) {
    if (to_be_added.canFind(i)) {
      if (to_be_removed.canFind(i)) {
        assert(set.is_in(i) == false);
      } else {
        assert(set.is_in(i) == true);
      }
    } else {
      assert(set.is_in(i) == false);
    }
  }

  //// Light all tests ////
  BitSet a = BitSet(23);
  a.light_all();
  assert(a.size() == 23);
  for (int i = 0; i < 23; ++i) {
    assert(a.is_in(i));
  }
}

// Equality Test

unittest {
  enum NELEMS = 34213;
  BitSet a = BitSet(NELEMS);
  BitSet b = BitSet(NELEMS);

  // Seed a random generator
  auto rnd = Random(42);

  int[] common_els;
  // Some might be duplicates
  common_els.length = NELEMS;
  for (int i = 0; i < NELEMS; ++i) {
    common_els[i] = uniform(0, NELEMS, rnd);
  }

  // Add them to `a`
  foreach (elem ; common_els) {
    a.add(elem);
  }
  auto shuffled = common_els.randomShuffle();
  // Add them in different order to `b`
  foreach (elem ; shuffled) {
    b.add(elem);
  }
  assert(a == b);
  
  // Same sizes but different contents
  BitSet c = BitSet(NELEMS);
  BitSet d = BitSet(NELEMS);

  c.add(1);
  c.add(22390);
  c.add(9789);

  d.add(1);
  c.add(9789);
  d.add(3232);

  assert(c != d);

  // Different sizes

  BitSet e = BitSet(3);
  BitSet f = BitSet(3);
  e.add(1);
  e.add(2);

  f.add(1);
  assert(e != f);
}

// Union tests

unittest {
  enum MAXELEMS = 1000;
  BitSet a = BitSet(MAXELEMS);
  BitSet b = BitSet(MAXELEMS);

  // Note that two of the elements (0, 298) are common
  // in the two sets ; Obviously this should be handled.
  // Also note that the two sets are of different sizes.
  int[] elements_of_a = [ 0, 63, 298, 874 ];
  int[] elements_of_b = [ 0, 329, 29, 298, 899, 19 ];

  // Add elements
  foreach (elem ; elements_of_a) {
    a.add(elem);
  }
  foreach (elem ; elements_of_b) {
    b.add(elem);
  }

  import std.algorithm : canFind;

  BitSet un1 = union_sets(a, b);
  long correct_size = 0;
  // Find the size pedantically
  for (int i = 0; i < MAXELEMS; ++i) {
    correct_size += elements_of_a.canFind(i) || elements_of_b.canFind(i);
  }
  assert(un1.size() == correct_size);
  // Check the size pedantically
  long sz = 0;
  for (int i = 0; i < MAXELEMS; ++i) {
    sz += un1.is_in(i);
  }
  assert(sz == correct_size);

  // Check the elements
  for (int i = 0; i < MAXELEMS; ++i) {
    if (un1.is_in(i)) {
      assert(elements_of_a.canFind(i) || elements_of_b.canFind(i));
    } else {
      assert(!elements_of_a.canFind(i) && !elements_of_b.canFind(i));
    }
  }

  BitSet empty = BitSet(MAXELEMS);
  BitSet un2 = union_sets(un1, empty);
  assert(un2 == un1);
}


// Intersection tests

unittest {
  enum MAXELEMS = 1000;
  BitSet a = BitSet(MAXELEMS);
  BitSet b = BitSet(MAXELEMS);

  // Note that two of the elements (0, 298) are common
  // in the two sets ; Obviously these should be the
  // only elements in the intersection.
  int[] elements_of_a = [ 0, 63, 298, 874 ];
  int[] elements_of_b = [ 0, 329, 29, 298, 899, 19 ];

  // Add elements
  foreach (elem ; elements_of_a) {
    a.add(elem);
  }
  foreach (elem ; elements_of_b) {
    b.add(elem);
  }

  import std.algorithm : canFind;

  BitSet inter1 = intersect_sets(a, b);
  long correct_size = 0;
  // Find the size pedantically
  for (int i = 0; i < MAXELEMS; ++i) {
    correct_size += elements_of_a.canFind(i) && elements_of_b.canFind(i);
  }
  assert(inter1.size() == correct_size);
  // Check the size pedantically
  long sz = 0;
  for (int i = 0; i < MAXELEMS; ++i) {
    sz += inter1.is_in(i);
  }
  assert(sz == correct_size);

  // Check the elements
  for (int i = 0; i < MAXELEMS; ++i) {
    if (inter1.is_in(i)) {
      assert(elements_of_a.canFind(i) && elements_of_b.canFind(i));
    }
  }

  BitSet empty = BitSet(MAXELEMS);
  BitSet inter2 = intersect_sets(inter1, empty);
  assert(inter2.size() == 0);
  for (int i = 0; i < MAXELEMS; ++i) {
    assert(!inter2.is_in(i));
  }
}


// Difference tests

unittest {
  enum MAXELEMS = 1000;
  BitSet a = BitSet(MAXELEMS);
  BitSet b = BitSet(MAXELEMS);

  // Note that two of the elements (0, 298) are common
  // in the two sets ; a - b should have all elements
  // of `a` except those 2.
  int[] elements_of_a = [ 0, 63, 298, 874 ];
  int[] elements_of_b = [ 0, 329, 29, 298, 899, 19 ];

  // Add elements
  foreach (elem ; elements_of_a) {
    a.add(elem);
  }
  foreach (elem ; elements_of_b) {
    b.add(elem);
  }

  BitSet diff1 = a - b;
  assert(diff1.size() == a.size() - 2);

  for (int i = 0; i < MAXELEMS; ++i) {
    if (diff1.is_in(i)) {
      assert(a.is_in(i) && !b.is_in(i));
    }
  }

  BitSet empty = BitSet(MAXELEMS);
  BitSet diff2 = a - empty;
  assert(diff2 == a);
}

// Copy tests

unittest {
  BitSet a = BitSet(200);
  BitSet b = BitSet(200);

  a.add(100);
  a.add(0);
  a.add(78);

  assert(b.copy(a));
  assert(b.is_in(100));
  assert(b.is_in(0));
  assert(b.is_in(78));

  // Add something to `b` and verify that `a`
  // didn't change.
  b.add(45);
  assert(!a.is_in(45));
}
