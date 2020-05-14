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

### [A Simple, Fast Dominance Algorithm - Keith Cooper, Timothy Harvey, Ken Kennedy](http://www.hipersoft.rice.edu/grads/publications/dom14.pdf)

This paper presents a presumably faster computation of dominators. Actually, it has the benefit that as a side-effect
one gets immediate dominators and sort of the dominator tree for free.

I'll follow-up with an article on this.

### [A Fast Algorithm for Finding Dominators in a Flowgraph - Thomas Lengauer, Robert Tarjan](https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.117.8843&rep=rep1&type=pdf)

This is an older algorithm and possibly the most used in production compilers.
