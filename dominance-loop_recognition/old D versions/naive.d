import std.container : Array;
import core.stdc.stdio : printf;
import std.stdio : writeln;
import bitset;

alias Succs = Array!int;

struct BasicBlock {
  Succs succs;
  Array!int preds;
  BitSet dominators;
}

struct CFG {
@nogc:

  void *mem;
  Array!BasicBlock bbs;

  @disable this();

  this(uint number_bbs) {
    // Assume that the user must have accounted for entry and exit.
    assert(number_bbs >= 2);
    import core.stdc.stdlib : calloc;

    bbs.length(number_bbs);

    size_t set_size = BitSet64.sizeof * num_words(number_bbs);
    mem = calloc(set_size, number_bbs);
    void *runner = mem;

    for (uint i = 0; i < number_bbs; ++i) {
      bbs[i].dominators = BitSet(number_bbs, runner);
      runner += set_size;
    }
  }

  // v -> w
  void add_edges(uint v, Succs succs) {
    assert(v < bbs.length);
    bbs[v].succs = succs;
    foreach (succ; succs) {
      assert(succ < bbs.length);
      bbs[succ].preds.insertBack(v);
    }
  }

  // We do have to return reference so that we can assign to it etc.
  ref BasicBlock opIndex(int i) { return bbs[i]; }

  ulong size() const { return bbs.length; }

  void free() {
    static import core.stdc.stdlib;
    core.stdc.stdlib.free(mem);
  }
}

private void postorder_helper(Array!int postorder, Array!bool visited,
                              CFG cfg, int v) {
  foreach (int child; cfg[v].succs) {
    if (!visited[child]) {
      visited[child] = true;
      postorder_helper(postorder, visited, cfg, child);
      postorder.insertBack(child);
    }
  }
}

// Post-order DFS traversal
private Array!int postorder_dfs(CFG cfg) {
  // Packed to use 1 bit per element.
  Array!bool visited;
  // Zero-initialized
  visited.length(cfg.size());
  visited[0] = true;

  Array!int postorder;
  // We don't include the entry node.
  postorder.reserve(cfg.size() - 1);

  postorder_helper(postorder, visited, cfg, 0);

  return postorder;
}

struct AlignedMemory {
  void *ptr;
  size_t offset;
};

AlignedMemory aligned_memory(T, uint alignment)(int nelems) {
import core.stdc.stdlib : malloc;
import core.stdc.string : memset;
  size_t size = T.sizeof *nelems;
  assert(size > 0);
  void *mem = malloc(size + alignment);
  assert(mem);
  memset(mem, 0, size + alignment);
  // Adjust memory pointer to be aligned
  size_t tmp = (cast(size_t) mem) & (alignment - 1);
  size_t ofs = (alignment - tmp) & (alignment - 1);
  mem = cast(void *)((cast(size_t) mem) + ofs);

  return AlignedMemory(mem, ofs);
}

void compute_dominators(CFG cfg) {
  Array!int postorder = postorder_dfs(cfg);

  // Initialize all the dominator sets except for the entry block
  // (i.e. 0)
  ulong sz = cfg.size();
  cfg[0].dominators.add(0);
  for (uint i = 1; i < cfg.size(); ++i) {
    cfg[i].dominators.light_all();
  }

  // Get aligned memory because a lot of memcpy / memset
  // will happen.
  AlignedMemory mem = aligned_memory !(BitSet64, 64)(cast(int) cfg.size());
  BitSet temp = BitSet(cast(int) cfg.size(), mem.ptr);
  bool change;
  do {
    change = false;
    foreach_reverse (int i; postorder) {
      BasicBlock *bb = &cfg[i];
      temp.light_all();
      foreach (pred; bb.preds) {
        temp = intersect_equal_sets_in_place(temp, cfg[pred].dominators);
      }
      temp.add(i);
      if (temp != bb.dominators) {
        change = true;
        assert(bb.dominators.copy(temp));
      }
    }
  } while (change);

  void *revert_offset = mem.ptr - mem.offset;
  import core.stdc.stdlib : free;
  free(revert_offset);
}

void print_bitset(BitSet s) {
  for (int i = 0; i < s.max_elems; ++i) {
    if (s.is_in(i))
      printf("%d ", i);
  }
  printf("\n");
}

void print_dominators(CFG cfg) {
  // Entry is by definition not dominated by any block
  for (uint i = 0; i < cfg.size(); ++i) {
    printf("%d: ", i);
    print_bitset(cfg[i].dominators);
  }
}

void main(string[] args) {
  import core.memory;
  GC.disable();

  import std.getopt : getopt;
  bool bench = false;
  getopt(args, "bench", &bench);

  if (bench) {
    benchmark_naive();
    return;
  }

  // Example

  const int number_of_basic_blocks = 8;
  CFG cfg = CFG(number_of_basic_blocks);

  cfg.add_edges(0, Succs(1));
  cfg.add_edges(1, Succs(2, 3));
  cfg.add_edges(2, Succs(7));
  cfg.add_edges(3, Succs(4));
  cfg.add_edges(4, Succs(5, 6));
  cfg.add_edges(5, Succs(7));
  cfg.add_edges(6, Succs(4));

  compute_dominators(cfg);
  print_dominators(cfg);
}

///////////////////// Benchmarks //////////////////////////

// Linear cfg i.e. BB_n -> BB_{n+1}
void linear_cfg(CFG cfg) {
  size_t sz = cfg.size();
  for (int i = 0; i < sz - 1; ++i) {
    cfg.add_edges(i, Succs(i + 1));
  }
}

// Alternate between:
// - BB_n -> BB_{n+1}
// - BB_n -> [ BB_{n+1}, BB_{n-1} ]
void fwdback_cfg(CFG cfg) {
  size_t sz = cfg.size();
  for (int i = 0; i < sz - 1; ++i) {
    if (i % 2 == 0) {
      cfg.add_edges(i, Succs(i + 1));
    } else {
      cfg.add_edges(i, Succs(i + 1, i - 1));
    }
  }
}

// genManyPred creates an array of blocks where 1/3rd have a successor of the
// first block, 1/3rd the last block, and the remaining third are plain.
void manypred_cfg(CFG cfg) {
  ulong sz = cfg.size();
  assert(sz > 0);
  for (int i = 0; i < sz - 1; ++i) {
    final switch (i % 3) {
    case 0:
      cfg.add_edges(i, Succs(i + 1));
      break;
    case 1:
      cfg.add_edges(i, Succs(i + 1, 0));
      break;
    case 2:
      cfg.add_edges(i, Succs(i + 1, cast(int) sz - 1));
      break;
    }
  }
}

void benchmark_naive_comp_and_count(CFG cfg, int nelems) {
  import core.time : MonoTime;
  auto start = MonoTime.currTime;
  compute_dominators(cfg);
  auto end = MonoTime.currTime;
  writeln("Benchmark Naive: ", nelems, " elements: ", end - start);
}

void benchmark_naive_linear(int nelems) {
  CFG cfg = CFG(nelems);
  linear_cfg(cfg);
  benchmark_naive_comp_and_count(cfg, nelems);
  cfg.free();
}

void benchmark_naive_fwdback(int nelems) {
  CFG cfg = CFG(nelems);
  fwdback_cfg(cfg);
  benchmark_naive_comp_and_count(cfg, nelems);
  cfg.free();
}

void benchmark_naive_manypred(int nelems) {
  CFG cfg = CFG(nelems);
  manypred_cfg(cfg);
  benchmark_naive_comp_and_count(cfg, nelems);
  cfg.free();
}

void benchmark_naive() {
  int[] set = [ 10, 50, 100, 200, 500, 800, 1000, 1500, 2000, 4000, 8000 ];
  writeln("--- Linear ---");
  foreach (num; set) { benchmark_naive_linear(num); }
  writeln();
  writeln("--- FwdBack ---");
  foreach (num; set) { benchmark_naive_fwdback(num); }
  writeln();
  writeln("--- ManyPred ---");
  foreach (num; set) { benchmark_naive_manypred(num); }
  writeln();
}
