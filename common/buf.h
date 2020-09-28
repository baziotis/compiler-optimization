#ifndef BUF_H
#define BUF_H

#include <limits>
#include <stdlib.h>

#include "stefanos.h"

template <typename T>
struct Buf {
  // Members
  size_t cap, _len;
  T *data;

  // Typedefs
  typedef T *iterator;
  typedef const T *const_iterator;

  Buf() {
    cap = _len = 0;
    data = nullptr;
  }

  Buf(size_t n) : Buf() { reserve(n); }

private:
  void _grow(size_t new_len) {
    constexpr size_t size_t_max = std::numeric_limits<size_t>::max();
    assert(cap <= (size_t_max - 1) / 2);
    size_t new_cap = MAX(MAX(2 * cap, new_len), 16);
    assert(new_len <= new_cap);
    assert(new_cap <= (size_t_max) / sizeof(T));
    size_t new_size = new_cap * sizeof(T);
    data = (T *)realloc(data, new_size);
    assert(data);
    cap = new_cap;
  }

public:
  
  void initialize() {
    for (T &t : *this) {
      new (&t) T();
    }
  }

  void push(T v) {
    size_t new_len = _len + 1;
    if (new_len > cap)
      _grow(new_len);
    data[_len] = v;
    _len = new_len;
  }

  void reserve(size_t n) {
    assert(_len == 0 && cap == 0);
    _grow(n);
    cap = n;
  }

  void reserve_and_set(size_t n) {
    reserve(n);
    _len = n;
  }

  ssize_t len() const {
    return _len;
  }

  void compact() {
    size_t old_len = _len;
    _grow(old_len);
    _len = cap = old_len;
  }

  void free() {
    if (data != nullptr)
      ::free(data);
    _len = 0;
    cap = 0;
  }

  Buf<T> deep_copy() {
    Buf<T> copy(cap);
    memcpy(copy.data, data, _len * sizeof(T));
    copy._len = _len;
    return copy;
  }

  void clear() { _len = 0; }

  const T &back() const {
    assert(_len >= 1);
    return data[_len - 1];
  }

  T &operator[](size_t i) { return data[i]; }
  const T &operator[](size_t i) const { return data[i]; }

  inline iterator begin() { return this->data; }
  inline const_iterator begin() const { return this->data; }

  inline iterator end() { return &this->data[_len]; }
  inline const_iterator end() const { return &this->data[_len]; }
};

#endif
