#pragma once

#include <iterator>
#include <tuple>
#include <vector>

/**
 * Data structure that behaves both like
 * multiple vectors and a single vector
 * to allow sharing between threads.
 *
 * Each chunk can be written to by a single thread.
 *
 * @tparam T the type to store
 */
template<class T>
class chunked_vector {

  template<class DerefType, class VectorRefType>
  class iterator_base;

 public:

  /**
   * Iterator that iterates the vector as
   * if it was not chunked.
   *
   * See the implementation below
   */
  using iterator = iterator_base<T&, chunked_vector*>;
  using const_iterator = iterator_base<const T&, const chunked_vector*>;

  /**
   * Constructs the chunked vector.
   *
   * @param chunk_cnt the number of chunks to create
   */
  chunked_vector(size_t chunk_cnt) :
    chunks_(chunk_cnt) {}

  /**
   * Find the number of elements.
   *
   * Thread safe due to not mutating anything.
   *
   * @return the number of elements in the vector
   *         in total (across all chunks)
   */
  size_t size() const {
    size_t res = 0;
    for (const auto &chunk : chunks_)
      res += chunk.size();
    return res;
  }

  /**
   * Remove all elements.
   *
   * Not thread safe.
   */
  void clear() {
    for (auto &chunk : chunks_)
      chunk.clear();
  }

  /**
   * @return an iterator pointing to the
   *         first element in the vector
   */
  iterator begin() {
    return iterator(this, 0);
  }

  /**
   * @return an iterator pointing past the
   *         last element in the vector
   */
  iterator end() {
    return iterator(this, size());
  }

  /**
   * Access a chunk.
   *
   * Modifying the chunk is thread safe
   * as long as any single chunk_index
   * is used by at most 1 thread.
   *
   * @param chunk_index the index of the chunk to access
   * @return            a reference to the chunk
   */
  std::vector<T> &chunk(size_t chunk_index) {
    return chunks_[chunk_index];
  }

 private:

  std::vector<std::vector<T>> chunks_;

};

/**
 * Iterator that iterates the vector as if it was not chunked.
 *
 * The operators have their standard meaning and can be
 * used as though they were iterators in a regular vector.
 *
 * @tparam DerefType     the type the iterator yields
 * @tparam VectorRefType the type of the reference to the iterated chunked_vector
 */
template<class T>
template<class DerefType, class VectorRefType>
class chunked_vector<T>::iterator_base
: public std::iterator<std::random_access_iterator_tag, T> {
  friend class chunked_vector;

  iterator_base(const VectorRefType vector_ref, size_t position) :
    vector_ref_(vector_ref),
    position_(position),
    chunk_id_(0),
    chunk_position_(position) { skip(); }

 public:

  /**
   * Default constructs an invalid
   * iterator pointing to nothing.
   */
  iterator_base() :
    vector_ref_(nullptr),
    position_(0),
    chunk_id_(0),
    chunk_position_(0) {}

  DerefType operator*() const {
    return vector_ref_->chunks_[chunk_id_][chunk_position_];
  }

  iterator &operator+=(ptrdiff_t offset) {
    position_ += offset;
    chunk_position_ += offset;
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

  DerefType operator[](ptrdiff_t offset) const {
    return *(*this + offset);
  }

  iterator operator++(int) {
    iterator cpy = *this;
    ++*this;
    return cpy;
  }

  iterator operator--(int) {
    iterator cpy = *this;
    --this;
    return cpy;
  }

  ptrdiff_t operator-(const iterator &o) const {
    return position_ - o.position_;
  }

  friend bool operator==(const iterator &l, const iterator &r) { return l.position_ == r.position_; }
  friend bool operator!=(const iterator &l, const iterator &r) { return l.position_ != r.position_; }
  friend bool operator< (const iterator &l, const iterator &r) { return l.position_ <  r.position_; }
  friend bool operator<=(const iterator &l, const iterator &r) { return l.position_ <= r.position_; }
  friend bool operator>=(const iterator &l, const iterator &r) { return l.position_ >= r.position_; }
  friend bool operator> (const iterator &l, const iterator &r) { return l.position_ >  r.position_; }

 private:

  const VectorRefType vector_ref_;
  size_t position_;
  size_t chunk_id_;
  ptrdiff_t chunk_position_;

  void skip() {
    // change to a further ahead chunk while
    // the position within the chunk
    // is greater than the size of the chunk
    while (chunk_id_ < vector_ref_->chunks_.size()
        && chunk_position_ >= (ptrdiff_t)vector_ref_->chunks_[chunk_id_].size())
      chunk_position_ -= vector_ref_->chunks_[chunk_id_++].size();

    // change to a further behind chunk while
    // the position within the chunk
    // is smaller than zero
    while (chunk_id_ > 0 && chunk_position_ < 0)
      chunk_position_ += vector_ref_->chunks_[--chunk_id_].size();
  }

};
