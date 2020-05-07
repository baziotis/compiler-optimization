import std.container : Array;
import bitset;
import core.stdc.stdio : printf;
import std.stdio : writeln;

alias Succs = Array!int;

struct BasicBlock {
  Succs succs;
  BitSet dominators;
  Array!int preds;
}

struct CFG {
@nogc: 

  uint num_bbs;
  void *mem;
  Array!BasicBlock bbs;

  @disable this();

  this(uint number_bbs) {
    // Assume that the user must have accounted for entry and exit.
    assert(number_bbs >= 2);
    import core.stdc.stdlib : calloc;

    num_bbs = number_bbs;
    bbs.reserve(num_bbs);
    bbs.length(num_bbs);

    size_t set_size = BitSet64.sizeof * num_words(number_bbs);
    mem = calloc(set_size, number_bbs);
    void *runner = mem;

    for (uint i = 0; i < num_bbs; ++i) {
      bbs[i].dominators = BitSet(number_bbs, runner);
      runner += set_size;
    }
  }

  // v -> w
  void add_edges(uint v, Succs succs) {
    assert(v < num_bbs);
    bbs[v].succs = succs;
    foreach (succ ; succs) {
      assert(succ < num_bbs);
      bbs[succ].preds.insertBack(v);
    }
  }

  // We do have to return reference so that we can assign to it etc.
  ref BasicBlock opIndex(int i) { return bbs[i]; }

  uint size() const {
    return num_bbs;
  }

  void free() {
    import core.stdc.stdlib : free;
    free(mem);
  }
}

struct AlignedMemory {
  void *ptr;
  size_t offset;
};

AlignedMemory aligned_memory(T, uint alignment)(int nelems) {
  import core.stdc.stdlib : malloc;
  import core.stdc.string : memset;
  size_t size = T.sizeof * nelems;
  assert(size > 0);
  void* mem = malloc(size + alignment);
  assert(mem);
  memset(mem, 0, size + alignment);
  // Adjust memory pointer to be aligned
  size_t tmp = (cast(size_t) mem) & (alignment - 1);
  size_t ofs = (alignment - tmp) & (alignment - 1);
  mem = cast(void*) ((cast(size_t)mem) + ofs);

  return AlignedMemory(mem, ofs);
}


void compute_dominators(CFG cfg, Array!int rev_postorder) {
  // Initialize all the dominator sets except for the entry block
  // (i.e. 0)
  uint sz = cfg.size();
  cfg[0].dominators.add(0);
  for (uint i = 1; i < cfg.size(); ++i) {
    cfg[i].dominators.light_all();
  }

  // Get aligned memory because a lot of memcpy / memset
  // will happen.
  AlignedMemory mem = aligned_memory!(BitSet64, 64)(cfg.size());
  BitSet temp = BitSet(cfg.size(), mem.ptr);
  bool change;
  do {
    change = false;
    foreach (int i ; rev_postorder) {
      BasicBlock *bb = &cfg[i];
      temp.light_all();
      foreach (pred ; bb.preds) {
        temp = intersect_equal_sets_in_place(temp, cfg[pred].dominators);
      }
      temp.add(i);
      if (temp != bb.dominators) {
        change = true;
        assert(bb.dominators.copy(temp));
        assert(cfg[i].dominators.nelems == temp.nelems);
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

void reverse_postorder_helper(Array!int rev_postorder, Array!bool visited, CFG cfg, int v) {
  foreach (int child ; cfg[v].succs) {
    if (!visited[child]) {
      visited[child] = true;
      rev_postorder.insertBack(child);
      reverse_postorder_helper(rev_postorder, visited, cfg, child);
    }
  }
}

// Reverse post-order DFS traversal
Array!int reverse_postorder_dfs(CFG cfg) {
  // Packed to use 1 bit per element.
  Array!bool visited;
  // Zero-initialized
  visited.length(cfg.size());
  visited[0] = true;

  Array!int rev_postorder;
  rev_postorder.reserve(cfg.size() - 1);

  reverse_postorder_helper(rev_postorder, visited, cfg, 0);

  return rev_postorder;
}

void main() {
  const int number_of_basic_blocks = 8;
  CFG cfg = CFG(number_of_basic_blocks);

  cfg.add_edges(0, Succs(1));
  cfg.add_edges(1, Succs(2, 3));
  cfg.add_edges(2, Succs(7));
  cfg.add_edges(3, Succs(4));
  cfg.add_edges(4, Succs(5, 6));
  cfg.add_edges(5, Succs(7));
  cfg.add_edges(6, Succs(4));

  Array!int rev_postorder = reverse_postorder_dfs(cfg);

  import core.time : MonoTime;
  auto start = MonoTime.currTime;
  compute_dominators(cfg, rev_postorder);
  auto end = MonoTime.currTime;
  print_dominators(cfg);
  writeln("\nTime: ", end - start);

  cfg.free();
}
