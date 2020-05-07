# Dominance - Loop Recognition

Algorithms (and Data Structures) for the computation of dominators, (post)dominator trees, dominance frontiers,
loop recognition etc.

## Dominators

### Naive Implementation

A naive but not completely dumb implementation of
the [Dataflow equation](https://en.wikipedia.org/wiki/Dominator_(graph_theory)#Algorithms) of dominator sets.

#### Noteworthy comments
The sets are represented as bitsets which leads to more compact representation and
way faster operations (than e.g. lists). Really the set implementation is more complicated than the algorithm itself.

Moreover, there is a minimal number of allocations.

### Naive Implementation
