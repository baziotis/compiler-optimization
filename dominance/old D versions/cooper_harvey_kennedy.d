import std.container : Array;
import core.stdc.stdio : printf;
import std.stdio : writeln;

/*
Note that here basic blocks don't contain the dominators.
Any dominance information is either explicitly or implicitly
obtained using the `idoms` array of the CFG. That is:
- To get an immediate dominator of a node, you just index the array
- To get all the dominators, you just loop the immediate dominators
  until you get to the entry.
- The dominator tree is implicit but also limited
  because we can only walk from leaves to the entry. Generally
  this is no problem because it let's us answer the question
  "Does `a` dominate `b`" -> Start from `b` and go up. If you see
  `a` then yes, otherwise no.
*/

alias Succs = Array!int;

struct BasicBlock {
  Succs succs;
  Array!int preds;
}

// Note: Maybe you could unify the `bbs` and `postorder` arrays.

struct CFG {
@nogc:

  enum UNDEFINED_IDOM = -1;
  Array!BasicBlock bbs;
  Array!int idoms;

  @disable this();

  this(int number_bbs) {
    // Assert that the user must have accounted for entry and exit.
    assert(number_bbs >= 2);
    assert(number_bbs < int.max);
    // Fills the elements with 0 value.
    bbs.length(number_bbs);

    idoms.length(number_bbs);
    idoms[0] = 0;
    for (ulong i = 1; i < number_bbs; ++i) {
      idoms[i] = -1;
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

  int size() const { return cast(int) bbs.length; }
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

void compute_dominators(CFG cfg) {
  Array!int postorder = postorder_dfs(cfg);
  Array!int postorder_map;
  int cfg_size = cfg.size();
  postorder_map.length(cfg.size());
  postorder_map[0] = cfg_size - 1;
  int counter = 0;
  foreach (p ; postorder) {
    postorder_map[p] = counter;
    ++counter;
  }

  bool change;
  do {
    change = false;
    foreach_reverse (int i; postorder) {
      BasicBlock *bb = &cfg[i];
      int new_idom = bb.preds[0];
      foreach (pred ; bb.preds[1..bb.preds.length]) {
        if (cfg.idoms[pred] != CFG.UNDEFINED_IDOM) {
          new_idom = intersect(new_idom, pred, cfg.idoms, postorder_map);
        }
      }
      if (cfg.idoms[i] != new_idom) {
        cfg.idoms[i] = new_idom;
        change = true;
      }
    }
  } while (change);
}

int intersect(int b1, int b2, Array!int idoms, Array!int postorder_map) {
  while (b1 != b2) {
    if (postorder_map[b1] < postorder_map[b2]) {
      b1 = idoms[b1];
    } else {
      b2 = idoms[b2];
    }
  }
  return b1;
}

void loop_and_print_dominators(CFG cfg, int bb) {
  int idom = bb;
  printf("%d", idom);
  if (idom == 0) {
    printf("\n");
    return;
  }
  do {
    idom = cfg.idoms[idom];
    printf(" %d", idom);
  } while (idom != 0);
  printf("\n");
}

void print_dominators(CFG cfg) {
  for (int i = 0; i < cfg.size(); ++i) {
    printf("%d: ", i);
    loop_and_print_dominators(cfg, i);
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
  int sz = cfg.size();
  for (int i = 0; i < sz - 1; ++i) {
    final switch (i % 3) {
    case 0:
      cfg.add_edges(i, Succs(i + 1));
      break;
    case 1:
      cfg.add_edges(i, Succs(i + 1, 0));
      break;
    case 2:
      cfg.add_edges(i, Succs(i + 1, sz - 1));
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
}

void benchmark_naive_fwdback(int nelems) {
  CFG cfg = CFG(nelems);
  fwdback_cfg(cfg);
  benchmark_naive_comp_and_count(cfg, nelems);
}

void benchmark_naive_manypred(int nelems) {
  CFG cfg = CFG(nelems);
  manypred_cfg(cfg);
  benchmark_naive_comp_and_count(cfg, nelems);
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
