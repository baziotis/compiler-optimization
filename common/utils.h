#ifndef UTILS
#define UTILS

#include "bitset.h"
#include "buf.h"
#include "cfg.h"

static
void postorder_helper(Buf<int> &postorder, BitSet visited, CFG cfg, int v) {
  for (int child : cfg.bbs[v].succs) {
    if (!bset_is_in(visited, child)) {
      bset_add(visited, child);
      postorder_helper(postorder, visited, cfg, child);
      postorder.push(child);
    }
  }
}

// Post-order DFS traversal
static
Buf<int> postorder_dfs(CFG cfg) {
  uint32_t cfg_size = cfg.size();
  BitSet visited = bset(cfg_size);
  bset_add(visited, 0);

  Buf<int> postorder;
  postorder.reserve(cfg_size);
  postorder_helper(postorder, visited, cfg, 0);
  // Don't forget to push the entry block, which
  // is not added by the helper.
  postorder.push(0);
  bset_free(visited);
  return postorder;
}

#endif
