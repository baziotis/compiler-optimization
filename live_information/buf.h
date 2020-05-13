#ifndef BUF
#define BUF

// Per Vognsen's implementation of Sean Barrett's stretchy buffer.

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// It's usual for the MAX to have been defined
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

typedef struct __BufHdr {
    uint32_t len;
    uint32_t cap;
    char buf[];
} __BufHdr;

#define buf__hdr(b) ((__BufHdr *)((char *)(b) - offsetof(__BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b)*sizeof(*b) : 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_reserve(b, n) ((n) <= buf_cap(b) ? 0 : ((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_reserve_and_set(b, n) (buf_reserve(b, n), buf__hdr(b)->len = n)
#define buf_push(b, ...) (buf_reserve((b), 1 + buf_len(b)), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

static
void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    assert(buf_cap(buf) <= (SIZE_MAX - 1)/2);
    size_t new_cap = MAX(2*buf_cap(buf), MAX(new_len, 16));
    assert(new_len <= new_cap);
    assert(new_cap <= (SIZE_MAX - offsetof(__BufHdr, buf))/elem_size);
    size_t new_size = offsetof(__BufHdr, buf) + new_cap*elem_size;
    __BufHdr *new_hdr;
    if (buf) {
        new_hdr = realloc(buf__hdr(buf), new_size);
        assert(new_hdr);
    } else {
        new_hdr = malloc(new_size);
        assert(new_hdr);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

#endif
