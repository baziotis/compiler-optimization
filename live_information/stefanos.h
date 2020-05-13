#ifndef STEFANOS
#define STEFANOS

#include <limits.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


#define ARR_LEN(a) (sizeof(a) / sizeof(*a))
// Branchless min -> Didn't really work in practice, around 0.6 the speed of the branching
// The reason is that the compiler generates `cmov` instruction for x86 which is way
// faster than doing all the above.
//#define min(x, y) (y ^ ((x ^ y) & -(x < y)))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

// Satured add is unfortunately and obviously slower than regular but
// fortunately not by much because the compiler will generate `cmov`
// and won't do actual branching.
// You may wish to replace this with normal addition if you know your data.
static int sadd(int x, int y) {
  // Avoid UB with unsigned addition
  int res = (unsigned int) x + y;
  if (res < 0)
    return INT_MAX;
  return res;
}

#define LOOP(it, s, e) \
  for (int it = (s); it < (e); ++it)

#define LOOPu32(it, s, e) \
  for (uint32_t it = (s); it < (e); ++it)

#include "buf.h"

#endif
