#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <forward_list>
#include <mutex>
#include <utility>
#include <vector>

/**
 * Hash set for use by multiple threads at once.
 *
 * The number of buckets is fixed,
 * so one should try and allocate an
 * appropriate amount beforehand.
 *
 * @tparam Key      The type to store in the set
 * @tparam Hash     Function-object type for hasing keys
 * @tparam KeyEqual Function-object type for checking key equality
 */
template<
 class Key,
 class Hash = std::hash<Key>,
 class KeyEqual = std::equal_to<Key>
> class thread_safe_set {
 public:

  /**
   * Constructs the thread-safe set.
   *
   * @param bits      Number of buckets will be 1<<bits
   * @param seed      Used in post-hash to make adversarial
   *                  input hard to create
   * @param hash      Instance to use of the Hash function-object type
   * @param key_equal Instance to use of the KeyEqual function-object type
   */
  thread_safe_set(
      int bits,
      uint64_t seed = std::chrono::steady_clock::now().time_since_epoch().count(),
      const Hash &hash = Hash(),
      const KeyEqual &key_equal = KeyEqual()) :
    fixed_random_(seed),
    hash_(hash),
    key_equal_(key_equal),
    bits_(bits),
    buckets_(size_t(1)<<bits) {}

  /**
   * Insert a key in a set if it is not there,
   * and return whether it was already there.
   *
   * @param  key The key to insert
   * @return true if the element was already there,
   *         false otherwise.
   */
  bool check_and_emplace(const Key &key) {
    size_t index = get_index(key);
    auto &[mutex, bucket] = buckets_[index];
    std::lock_guard<std::mutex> lock(mutex);

    auto already = std::find_if(bucket.begin(), bucket.end(), [&](auto &&in_bucket) {
      return key_equal_(in_bucket, key);
    });

    if (already != bucket.end())
      return true;
    
    bucket.push_front(key);
    return false;
  }

 private:

  const uint64_t fixed_random_;
  Hash hash_;
  KeyEqual key_equal_;

  const int bits_;
  std::vector<std::pair<std::mutex, std::forward_list<Key>>> buckets_;

  static uint64_t splitmix64(uint64_t x) {
    // http://xorshift.di.unimi.it/splitmix64.c
    x += 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
  }

  // find the index of the bucket given a key
  size_t get_index(const Key &key) {
    uint64_t hash = splitmix64(hash_(key) + fixed_random_);
    return hash & ((size_t(1) << bits_) - 1);
  }

};
