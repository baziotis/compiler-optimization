# Compiler Optimization

Several experiments and implementations relevant in compiler optimization.

For this purpose, I created a very simple IR that is described in `/IR` directory
along with some examples.

I try to create .h files that have the available utilities for a specific analysis / transformation (e.g.
dtree.h contains utilities for the dominator tree). These are not single-file `.h` files since they
depend on one another and most of all, the depend on the `.h` files in `/common`. But they provide
a very flexible way of combining them in this repository to create more advanced optimizations.

Every sub-directory has its own README. Apart from the `.h` files which contain the most important
work, there are some `.c` files in the directories that are used to easily try using some
utilities (e.g. `print_liveout.c` uses liveout.h to print liveout information for a `.ir` file)
