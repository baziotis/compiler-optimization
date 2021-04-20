#ifndef LENGAUER_TARJAN_H
#define LENGAUER_TARJAN_H

#include <stdio.h>

#include "../common/bitset.h"
#include "../common/stack.h"
#include "../common/cfg.h"
#include "../common/parser_ir.h"

#define UNDEFINED_BBNUM -1
#define DEBUG_LT 0

#if DEBUG_LT

#define DBG_BLK(x) x
static char bbnum_to_letter[32001];

#else

#define DBG_BLK(x)

#endif


// Buf<T> already has `initialize()` but I wanted to be sure
// we're using memset.
static void
zero_buf(Buf<int> b) {
  memset(b.data, 0, b.len() * sizeof(int));
}

// Iterative version of the paper DFS. It does DFS and also initializes
// the arrays.
void custom_dfs(CFG cfg, Buf<int> bbnum_to_semi_dfnum, Buf<int> dfnum_to_bbnum,
                Buf<int> bbnum_to_parent_bbnum, Buf<int> bbnum_to_ancestor_bbnum,
                Buf<int> bucket_head, Buf<int> bucket_link) {
  Stack<int> stack;
  // Insert the entry block
  stack.push(0);

  zero_buf(bbnum_to_semi_dfnum);
  int n = 0;
  while (!stack.empty()) {
    int curr_bbnum = stack.pop();

    if (bbnum_to_semi_dfnum[curr_bbnum] != 0) { // visited
      continue;
    }
    ++n;
    bbnum_to_semi_dfnum[curr_bbnum] = n;
    dfnum_to_bbnum[n] = curr_bbnum;

    bbnum_to_ancestor_bbnum[curr_bbnum] = UNDEFINED_BBNUM;
    bucket_head[curr_bbnum] = UNDEFINED_BBNUM;
    bucket_link[curr_bbnum] = UNDEFINED_BBNUM;
    // Go in reverse to push in an order that helps with
    // the examples.
    LOOP_REV(succ_idx, 0, cfg.bbs[curr_bbnum].succs.len()) {
      int succ_bbnum = cfg.bbs[curr_bbnum].succs[succ_idx];
      if (bbnum_to_semi_dfnum[succ_bbnum] == 0) { // unvisited
        stack.push(succ_bbnum);
        // Theoretically, `succ_bbnum` could be pushed multiple times.
        // It's not a problem for the DFS because we won't visit it twice.
        // Also, its parent is always going to be `bbnum`.
        bbnum_to_parent_bbnum[succ_bbnum] = curr_bbnum;
      }
    }
  }

  stack.free();
}

static int
ancestor_with_lowest_semi(int bbnum, Buf<int> bbnum_to_semi_dfnum, Buf<int> dfnum_to_bbnum, Buf<int> bbnum_to_ancestor_bbnum) {
  int best_bbnum = bbnum;
  int curr_bbnum = bbnum;
  while (bbnum_to_ancestor_bbnum[curr_bbnum] != UNDEFINED_BBNUM) {
    if (bbnum_to_semi_dfnum[curr_bbnum] < bbnum_to_semi_dfnum[best_bbnum]) {
      best_bbnum = curr_bbnum;
    }
    curr_bbnum = bbnum_to_ancestor_bbnum[curr_bbnum];
  }

  return best_bbnum;
}

static void
link(int v, int w, Buf<int> bbnum_to_ancestor_bbnum) {
  bbnum_to_ancestor_bbnum[w] = v;
}

void lt_slow(CFG cfg, Buf<int> idom) {
  int nelems = cfg.size();

  Buf<int> bbnum_to_semi_dfnum;
  Buf<int> dfnum_to_bbnum;
  Buf<int> bbnum_to_parent_bbnum;
  Buf<int> bbnum_to_ancestor_bbnum;
  Buf<int> bucket_head;
  Buf<int> bucket_link;
  bbnum_to_semi_dfnum.reserve_and_set(nelems);
  dfnum_to_bbnum.reserve_and_set(nelems + 1);
  bbnum_to_parent_bbnum.reserve_and_set(nelems);
  bbnum_to_ancestor_bbnum.reserve_and_set(nelems);
  bucket_head.reserve_and_set(nelems);
  bucket_link.reserve_and_set(nelems);

  custom_dfs(cfg, bbnum_to_semi_dfnum, dfnum_to_bbnum,
             bbnum_to_parent_bbnum, bbnum_to_ancestor_bbnum,
             bucket_head, bucket_link);
  
  // Note that dfnumbering is in [1, nelems].
  for (int dfnum = nelems; dfnum >= 2; --dfnum) {
    int w = dfnum_to_bbnum[dfnum];
    // Remember that semi(w), before the semidominator is computed,
    // is the dfnum of w.
    assert(bbnum_to_semi_dfnum[w] == dfnum);
    DBG_BLK(assert(w < sizeof(bbnum_to_letter));)
    DBG_BLK(printf("w: %c\n", bbnum_to_letter[w]);)
    int parent = bbnum_to_parent_bbnum[w];
    DBG_BLK(printf("  parent: %c\n", bbnum_to_letter[parent]);)

    int best_semi = bbnum_to_semi_dfnum[w];
    for (int pred_bbnum : cfg.bbs[w].preds) {
      DBG_BLK(printf("  pred: %c\n", bbnum_to_letter[pred_bbnum]);)
      int u = ancestor_with_lowest_semi(pred_bbnum, bbnum_to_semi_dfnum, dfnum_to_bbnum, bbnum_to_ancestor_bbnum);
      DBG_BLK(printf("  u: %c\n", bbnum_to_letter[u]);)
      int semi_u = bbnum_to_semi_dfnum[u];
      DBG_BLK(printf("  semi_u (dfnum): %d\n", semi_u);)
      DBG_BLK(printf("  semi_u (letter): %c\n", bbnum_to_letter[dfnum_to_bbnum[semi_u]]);)
      DBG_BLK(printf("  best_semi (dfnum): %d\n", best_semi);)
      DBG_BLK(printf("  best_semi (letter): %c\n", bbnum_to_letter[dfnum_to_bbnum[best_semi]]);)
      if (semi_u < best_semi)
        best_semi = semi_u;
    }
    bbnum_to_semi_dfnum[w] = best_semi;
    DBG_BLK(printf("  semi[w] = %c\n", bbnum_to_letter[dfnum_to_bbnum[best_semi]]);)
    link(parent, w, bbnum_to_ancestor_bbnum);

    // Add `w` to the bucket of semi(w)
    int semi_w_bbnum = dfnum_to_bbnum[bbnum_to_semi_dfnum[w]];
    // Note that a `bucket_head`, for a specific bb,
    // may be set multiple times but a `bucket_link` not. That
    // is because every `w` has exactly one semi-dominator. This
    // helps us not care about links and "empty" a bucket
    // by just resetting the `bucket_head` of the relevant semi.
    bucket_link[w] = bucket_head[semi_w_bbnum];
    bucket_head[semi_w_bbnum] = w;
    
    // Process the bucket of parent(w) because we're finished
    // with this branch.
    for (int bbnum = bucket_head[parent]; bbnum != UNDEFINED_BBNUM;
         bbnum = bucket_link[bbnum]) {
      DBG_BLK(printf("  samedom: %c\n", bbnum_to_letter[bbnum]);)
      int u = ancestor_with_lowest_semi(bbnum, bbnum_to_semi_dfnum,
                                        dfnum_to_bbnum,
                                        bbnum_to_ancestor_bbnum);
      if (bbnum_to_semi_dfnum[u] < bbnum_to_semi_dfnum[bbnum]) {
        idom[bbnum] = u;
      } else {
        // Remember: parent[w] is bbnum's semidom.
        idom[bbnum] = parent;
      }
    }
    // "Empty" the bucket(p). Note that links (i.e., bucket_link)
    // will still be pointing to wherever they were pointing
    // but that is not a problem.
    bucket_head[parent] = UNDEFINED_BBNUM;
  }

  for (int dfnum = 2; dfnum < nelems; ++dfnum) {
    int w = dfnum_to_bbnum[dfnum];
    if (idom[w] != dfnum_to_bbnum[bbnum_to_semi_dfnum[w]]) {
      int u = idom[w];
      // `u` must be smaller than `w` and thus its
      // idom has already been computed.
      assert(u < w);
      idom[w] = idom[u];
    }
  }

  bbnum_to_semi_dfnum.free();
  dfnum_to_bbnum.free();
  bbnum_to_parent_bbnum.free();
  bbnum_to_ancestor_bbnum.free();
  bucket_head.free();
  bucket_link.free();

}

#undef UNDEFINED_BBNUM

#if DEBUG_LT

int find_bbnum_for_letter(CFG cfg, char l) {
  int idx = -1;
  for (int i = 0; i < cfg.size(); ++i) {
    if (bbnum_to_letter[i] == l) {
      idx = i;
      break;
    }
  }
  assert(idx != -1);
  return idx;
}

void add_letter_to_letter_edge(CFG cfg, char a, char b) {
  int bbnum_a = find_bbnum_for_letter(cfg, a);
  int bbnum_b = find_bbnum_for_letter(cfg, b);
  cfg.add_edge(bbnum_a, bbnum_b);
}

void name_bbs(CFG cfg) {
  // R must be the root, to follow the paper convention
  bbnum_to_letter[0] = 'R';
  int bbnum;
  char letter;
  for (bbnum = 1, letter = 'A'; bbnum < cfg.size(); ++letter, ++bbnum) {
    if (letter == 'R')
      continue;
    bbnum_to_letter[bbnum] = letter;
  }
}

void print_dominators(Buf<int> idom) {
  printf("\n-------------------------------------------\n\n");
  for (int bbnum = 0; bbnum < idom.len(); ++bbnum) {
    int letter = bbnum_to_letter[bbnum];
    int idomi = idom[bbnum];
    int idom_letter = bbnum_to_letter[idomi];
    printf("%c -> %c\n", idom_letter, letter);
  }
}

#endif

#endif
