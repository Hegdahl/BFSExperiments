#pragma once

#include <iterator>
#include <tuple>
#include <vector>

/**
 * Data structure that behaves both like
 * multiple vectors and a single vector
 * to allow sharing between threads.
 *
 * @tparam T the type to store
 */
template<class T>
class chunked_vector {
 public:
  std::vector<std::vector<T>> chunks;

  chunked_vector(size_t chunk_cnt)
    : chunks(chunk_cnt) {}

  class iterator : public std::iterator<std::random_access_iterator_tag, T> {
    chunked_vector *sv;
    size_t pos;
    size_t chunk_id;
    ptrdiff_t chunk_pos;

    void skip() {
      while (chunk_id < sv->chunks.size() && chunk_pos >= (ptrdiff_t)sv->chunks[chunk_id].size())
        chunk_pos -= sv->chunks[chunk_id++].size();
      while (chunk_id > 0 && chunk_pos < 0)
        chunk_pos += sv->chunks[--chunk_id].size();
    }

   public:
    iterator()
      : sv(nullptr), pos(0), chunk_id(0), chunk_pos(0) {}
    iterator(chunked_vector *sv_, size_t pos_)
      : sv(sv_), pos(pos_), chunk_id(0), chunk_pos(pos_) {
      skip();
    }

    T &operator*() const {
      return sv->chunks[chunk_id][chunk_pos];
    }

    iterator &operator+=(ptrdiff_t offset) {
      pos += offset;
      chunk_pos += offset;
      skip();
      return *this;
    }

    iterator &operator-=(ptrdiff_t offset) {
      return *this += -offset;
    }

    iterator &operator++() {
      return *this += 1;
    }

    iterator &operator--() {
      return *this -= 1;
    }

    iterator operator+(ptrdiff_t offset) const {
      iterator cpy = *this;
      cpy += offset;
      return cpy;
    }

    iterator operator-(ptrdiff_t offset) const {
      iterator cpy = *this;
      cpy -= offset;
      return cpy;
    }

    friend iterator operator+(ptrdiff_t offset, const iterator &it) {
      return it + offset;
    }

    T &operator[](ptrdiff_t offset) const {
      return *(*this + offset);
    }

    iterator operator++(int) const {
      iterator cpy = *this;
      ++this;
      return cpy;
    }

    iterator operator--(int) const {
      iterator cpy = *this;
      --this;
      return cpy;
    }

    ptrdiff_t operator-(const iterator &o) const {
      return pos - o.pos;
    }

    friend bool operator==(const iterator &l, const iterator &r) { return l.pos == r.pos; }
    friend bool operator!=(const iterator &l, const iterator &r) { return l.pos != r.pos; }
    friend bool operator< (const iterator &l, const iterator &r) { return l.pos <  r.pos; }
    friend bool operator<=(const iterator &l, const iterator &r) { return l.pos <= r.pos; }
    friend bool operator>=(const iterator &l, const iterator &r) { return l.pos >= r.pos; }
    friend bool operator> (const iterator &l, const iterator &r) { return l.pos >  r.pos; }

  };

  iterator begin() {
    return iterator(this, 0);
  }

  iterator end() {
    return iterator(this, size());
  }

  size_t size() const {
    size_t res = 0;
    for (const auto &chunk : chunks)
      res += chunk.size();
    return res;
  }

  void clear() {
    for (auto &chunk : chunks)
      chunk.clear();
  }

};
