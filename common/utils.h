#ifndef UTILS
#define UTILS

#include "bitset.h"
#include "buf.h"
#include "cfg.h"

static
void postorder_helper(int *postorder, BitSet visited, CFG cfg, int v) {
  uint32_t len = buf_len(cfg.bbs[v].succs);
  LOOPu32(c, 0, len) {
    int child = cfg.bbs[v].succs[c];
    if (!bset_is_in(visited, child)) {
      bset_add(visited, child);
      postorder_helper(postorder, visited, cfg, child);
      buf_push(postorder, child);
    }
  }
}

// Post-order DFS traversal
static
int *postorder_dfs(CFG cfg) {
  uint32_t cfg_size = buf_len(cfg.bbs);
  BitSet visited = bset(cfg_size);
  bset_add(visited, 0);

  int *postorder = NULL;
  buf_reserve(postorder, cfg_size);
  postorder_helper(postorder, visited, cfg, 0);
  // Don't forget to push the entry block, which
  // is not added by the helper.
  buf_push(postorder, 0);
  bset_free(visited);
  return postorder;
}

#endif
