# Simplified IR

This IR was created for the purposes of this live variable analysis solver
(although it will probably be used in future optimizations too).
Its purpose is to be as simple as possible but at the same time not _too_ simple so that the analyses
and transformations on it are actually meaningful.

# Syntax and Structure

First of all, this is not SSA (hopefully in the future I'll make a SSA builder).
Then in this IR, only a single procedure is described per file. This CFG is made
from BasicBlocks, which are the top-level entity. The rest is described in detail
below. **Warning**: A lot of terminology is copied from LLVM. That initially was done
on purpose but the way it ended up is quite different, especially for `Instruction`s
and `Value`s.

## BasicBlock
  A `BasicBlock` is just an array of `Instruction`s. Ideally, it must end
  with some kind of transfer of control-flow (for us, only branches, i.e. `BR` instructions),
  although this is not enforced currently.
  
  A `BasicBlock` is started with the syntax `.<integer>:`. The `<integer>` is the number of the
  block and each `BasicBlock` has to have as a number exactly +1 from the prevous, except
  for the first that starts with `0` (this is an arbitrary constraint, probably it will be removed).
  
  This is also the way with which we target `BasicBlock`s in `BR` (branch) instructions, see below.

## Instruction
  It can either be a `DEF`, a `PRINT` or a `BR`. `DEF` is used for the obvious purpose
  of assigning to a "variable" (see `Value` below) and it is key in live analysis information.
  `PRINT` is just a whatever generic instruction whose purpose is to have
  an instruction that only uses variables and doesn't define.
  BR is used as a branch instruction, whose purpose is really only to have
  an automatic way of adding successors and predecessors.
  Instructions have different fields according to their kind:
  
  - `DEF`:
     This `Instruction` defines a register, which is the analogous of assigning to a variable in C-like
     programming languages. Each register is identified by an integer and is printed like this `%<integer>`.
     Each register can be defined more than once so it really is more of an assignment than a definition
     but I just use with the standard terminlogy (which makes more sense for later, i.e. SSA).
     A `DEF` instruction, apart from the register it defines, has a RHS `Operation`
     that is to the right of the "arrow", i.e. the `Operation` we assign. Examples:<br/>
     `%0 <- 1`<br/>
     `%5 <- 2 + %6`
  - `PRINT`:
     It has an `Operation` that can only be `OP_SIMPLE` for now (i.e. `Value`). There's
     no particular reason that it is only `OP_SIMPLE`, probably I'll remove this constraint
     soon.Examples:<br/>
     `PRINT 2`<br/>
     `PRINT %0`
  - `BR`:
    It can either be conditional or unconditional:
    - `BR_UNCOND`:
      For unconditional branches, we only need a label / BB, which we target. We access it with
      the `uncond_lbl` field. Example: `BR .0`.
    - `BR_COND`:
      For conditional branches, we need 3 things. The `Value` that we check, which
      is accessed with `cond_val`, and the two BBs, which are accessed with
      `then` and `els`. Examples:<br/>
      `BR 0, .L0, .L1`<br/>
      `BR %0, .L0, .L1` but **not**<br/>
      `BR 0 + 1, .L0, .L1`
      
## Operation
  An `Operation` is either a three-address operation, i.e. x OPERATOR y or
  or just a SIMPLE `x`. For now, OPERATOR is only addition, i.e. ADD.
  `x`, `y` are `Value`s. An `Operation` is the only thing that can appear
  at the right of a `DEF`. I wanted a simple way to distinguish complex
  expressions (like addition) from simple scalar entities (like `Value`s).

## Value
  A `Value` is either a register (e.g. %1) or an immediate integer (e.g. 1).
  A `Value` has only an integer type and because it is the building block of everything
  else, **everything is constrained only to one single integer type**.
  A `Value` is a `uint32_t` in which the lower 31 bits are used for either register / immediate
  value. The MSB is used to signify whether it's a register or an immediate.
  
# Examples

See the `.ir` files in this folder for some examples. Some of them have the CFG draw too and possible
C source code that could have generated it.
You can also see the `.c` files in which we build the CFG by hand, using constructors for everything
referenced above.
