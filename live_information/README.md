# Live Information

This is an implementation of the LiveOut Data-flow equations.

## Background

Computing the live variables in a block is a very common problem in compiler optimization and data-flow
analysis. This is not a tutorial on the theoritical aspect of data-flow analysis or live information
but if you want to learn more, you can read Chapters 8 and 9 from the book
[Engineering a Compiler, 2nd Edition](https://www.elsevier.com/books/engineering-a-compiler/cooper/978-0-12-088478-0)

## Compile and Run

**Compile**: Use the script `./compile.sh`. It outputs an executable called `liveout`<br/>
**Run**: `./liveout <filename>.ir`

As it is apparent from above, the LiveOut solver gets as input a single `.ir` file. These files are supposed
to contain a custom stripped-down IR I made for the purposes of this solver. For more information about the
IR, see the folder `./examples`.

The solver outputs the live variables at the exit for each basic block in the CFG.
