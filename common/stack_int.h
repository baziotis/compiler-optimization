#ifndef STACK_INT
#define STACK_INT

#include "buf.h"

// Dynamic stack over a stretchy buffer.
typedef struct StackInt {
  int curr;
  int *elems;
} StackInt;

static
StackInt stack_construct(void) {
  StackInt s = { .curr = 0, .elems = NULL };
  return s;
}

static
int stack_pop(StackInt *s) {
  assert(s->curr > 0);
  s->curr -= 1;
  return s->elems[s->curr];
}

static
void stack_push(StackInt *s, int n) {
  if (s->curr == buf_len(s->elems)) {
    buf_push(s->elems, n);
    s->curr += 1;
    return;
  }
  assert(s->curr < buf_len(s->elems));
  s->elems[s->curr] = n;
  s->curr += 1;
}

static
int stack_empty(StackInt *s) {
  return s->curr == 0;
}

static
void stack_free(StackInt *s) {
  buf_free(s->elems);
}

#endif
