#include "lengauer-tarjan.h"

// You have to define DEBUG_LT to 1 in lengauer-tarjan.h

void example1(void) {
  CFG cfg(8);

  name_bbs(cfg);

  add_letter_to_letter_edge(cfg, 'R', 'A');

  add_letter_to_letter_edge(cfg, 'A', 'B');
  add_letter_to_letter_edge(cfg, 'A', 'C');

  add_letter_to_letter_edge(cfg, 'B', 'C');
  add_letter_to_letter_edge(cfg, 'B', 'F');

  add_letter_to_letter_edge(cfg, 'C', 'D');

  add_letter_to_letter_edge(cfg, 'D', 'E');

  add_letter_to_letter_edge(cfg, 'F', 'G');

  add_letter_to_letter_edge(cfg, 'G', 'E');


  Buf<int> idom;
  idom.reserve_and_set(cfg.size());
  lt(cfg, idom);
  print_dominators(idom);

  cfg.destruct();
}

void paper_example() {
  CFG cfg(13);

  name_bbs(cfg);

  add_letter_to_letter_edge(cfg, 'R', 'A');
  add_letter_to_letter_edge(cfg, 'R', 'B');
  add_letter_to_letter_edge(cfg, 'R', 'C');

  add_letter_to_letter_edge(cfg, 'A', 'D');

  add_letter_to_letter_edge(cfg, 'B', 'A');
  add_letter_to_letter_edge(cfg, 'B', 'D');
  add_letter_to_letter_edge(cfg, 'B', 'E');

  add_letter_to_letter_edge(cfg, 'C', 'F');
  add_letter_to_letter_edge(cfg, 'C', 'G');

  add_letter_to_letter_edge(cfg, 'D', 'L');

  add_letter_to_letter_edge(cfg, 'E', 'H');

  add_letter_to_letter_edge(cfg, 'F', 'I');

  add_letter_to_letter_edge(cfg, 'G', 'I');
  add_letter_to_letter_edge(cfg, 'G', 'J');

  add_letter_to_letter_edge(cfg, 'H', 'E');
  add_letter_to_letter_edge(cfg, 'H', 'K');

  add_letter_to_letter_edge(cfg, 'I', 'K');

  add_letter_to_letter_edge(cfg, 'J', 'I');

  add_letter_to_letter_edge(cfg, 'K', 'R');
  add_letter_to_letter_edge(cfg, 'K', 'I');

  add_letter_to_letter_edge(cfg, 'L', 'H');


  Buf<int> idom;
  idom.reserve_and_set(cfg.size());

  lt(cfg, idom);
  print_dominators(idom);

  cfg.destruct();
}

// You have to define DEBUG_LT to 1 in lengauer-tarjan.h

int main(int argc, char **argv) {
  example1();
}
