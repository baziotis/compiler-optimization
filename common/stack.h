#ifndef STACK
#define STACK

#include "buf.h"

// Dynamic stack over a stretchy buffer.
template <typename T>
struct Stack {
  int curr;
  Buf<T> elems;

  Stack() {
    curr = 0;
  }

  T pop() {
    assert(curr > 0);
    curr -= 1;
    return elems[curr];
  }

  void push(T n) {
    if (curr == elems.len()) {
      elems.push(n);
      curr += 1;
      return;
    }
    assert(curr < elems.len());
    elems[curr] = n;
    curr += 1;
  }

  bool empty() const {
    return curr == 0;
  }

  void free() {
    elems.free();
  }
};

#endif
