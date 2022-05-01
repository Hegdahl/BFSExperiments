#include <cassert>
#include <ios>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <random>

#include "bfs.hpp"
#include "time.hpp"

unsigned max_len;

void set_max_len(unsigned new_max_len) {
  max_len = new_max_len;
  std::cout << "max_len: " << max_len << std::endl;
}

/**
 * Dummy object to test potential
 * speedup from parallelism.
 */
struct S {

  std::vector<int> a;

  friend bool operator==(const S &l, const S &r) {
    return l.a == r.a;
  }

};

std::vector<S> cheap_sparse(const S &s) {
  std::vector<S> transitions;

  if (s.a.size() < max_len) {
    transitions.push_back(s);
    transitions.back().a.push_back(0);
    transitions.push_back(s);
    transitions.back().a.push_back(1);
  }

  if (s.a.size() > 0) {
    transitions.push_back(s);
    transitions.back().a.pop_back();

    if (s.a.size() < max_len) {
      for (int bit0 : {0, 1})
        for (int bit1 : {0, 1}) {
          transitions.push_back(s);
          transitions.back().a.pop_back();
          transitions.back().a.push_back(bit0);
          transitions.back().a.push_back(bit1);
        }
    }
  }

  return transitions;
}

std::vector<S> cheap_dense(const S &s) {
  std::vector<S> transitions;
  if (s.a.size() < max_len) {
    transitions.push_back(s);
    transitions.back().a.push_back(0);
  }

  unsigned as_bits = 0;
  for (int i : s.a)
    as_bits = as_bits << 1U | i;

  unsigned mask = ~as_bits & ((1U << s.a.size()) - 1);
  
  for (unsigned activate = mask; activate != 0; activate = (activate-1) & mask) {
    transitions.push_back(s);
    auto &a = transitions.back().a;
    for (int i = 0; i < (int)a.size(); ++i)
      a[i] ^= activate >> (a.size() - i - 1) & 1;
  }

  return transitions;
}

auto expensive_sparse(const S &s) {
  auto transitions = cheap_sparse(s);

  std::mt19937 rng(123);
  for (int rep = 0; rep < 100; ++rep)
    std::shuffle(transitions.begin(), transitions.end(), rng);

  return transitions;
}

namespace std {

/**
 * Extend std to make S hashable.
 */
template <>
struct hash<S> {
  size_t operator()(const S &s) const {
    size_t res = 0;
    for (int bit : s.a) res = res << 1 | bit;
    return res;
  }
};

}  // namespace std

int main() {
  std::cout
      << "format: bfs_type(transition_type, source, threads [, log2(set_size)])\n"
      << std::endl;
  
  //*
  set_max_len(20);
  TIME(sequential_bfs(cheap_sparse, S{}));
  //TIME(bfs_phmap(cheap_sparse, S{}, 64));
  TIME(bfs_phmap(cheap_sparse, S{}, 32));
  TIME(bfs_phmap(cheap_sparse, S{}, 16));
  TIME(bfs_phmap(cheap_sparse, S{}, 8));
  TIME(bfs_phmap(cheap_sparse, S{}, 4));
  TIME(bfs_phmap(cheap_sparse, S{}, 2));
  TIME(bfs_phmap(cheap_sparse, S{}, 1));
  //TIME(bfs_fixed_size_set(cheap_sparse, S{}, 64, max_len));
  TIME(bfs_fixed_size_set(cheap_sparse, S{}, 32, max_len));
  TIME(bfs_fixed_size_set(cheap_sparse, S{}, 16, max_len));
  TIME(bfs_fixed_size_set(cheap_sparse, S{}, 8, max_len));
  TIME(bfs_fixed_size_set(cheap_sparse, S{}, 4, max_len));
  TIME(bfs_fixed_size_set(cheap_sparse, S{}, 2, max_len));
  TIME(bfs_fixed_size_set(cheap_sparse, S{}, 1, max_len));
  std::cout << std::endl;
  // */

  //*
  set_max_len(15);
  TIME(sequential_bfs(cheap_dense, S{}));
  //TIME(bfs_phmap(cheap_dense, S{}, 64));
  TIME(bfs_phmap(cheap_dense, S{}, 32));
  TIME(bfs_phmap(cheap_dense, S{}, 16));
  TIME(bfs_phmap(cheap_dense, S{}, 8));
  TIME(bfs_phmap(cheap_dense, S{}, 4));
  TIME(bfs_phmap(cheap_dense, S{}, 2));
  TIME(bfs_phmap(cheap_dense, S{}, 1));
  //TIME(bfs_fixed_size_set(cheap_dense, S{}, 64, max_len));
  TIME(bfs_fixed_size_set(cheap_dense, S{}, 32, max_len));
  TIME(bfs_fixed_size_set(cheap_dense, S{}, 16, max_len));
  TIME(bfs_fixed_size_set(cheap_dense, S{}, 8, max_len));
  TIME(bfs_fixed_size_set(cheap_dense, S{}, 4, max_len));
  TIME(bfs_fixed_size_set(cheap_dense, S{}, 2, max_len));
  TIME(bfs_fixed_size_set(cheap_dense, S{}, 1, max_len));
  std::cout << std::endl;
  // */

  //*
  set_max_len(20);
  TIME(sequential_bfs(expensive_sparse, S{}));
  //TIME(bfs_phmap(expensive_sparse, S{}, 64));
  TIME(bfs_phmap(expensive_sparse, S{}, 32));
  TIME(bfs_phmap(expensive_sparse, S{}, 16));
  TIME(bfs_phmap(expensive_sparse, S{}, 8));
  TIME(bfs_phmap(expensive_sparse, S{}, 4));
  TIME(bfs_phmap(expensive_sparse, S{}, 2));
  TIME(bfs_phmap(expensive_sparse, S{}, 1));
  //TIME(bfs_fixed_size_set(expensive_sparse, S{}, 64, max_len));
  TIME(bfs_fixed_size_set(expensive_sparse, S{}, 32, max_len));
  TIME(bfs_fixed_size_set(expensive_sparse, S{}, 16, max_len));
  TIME(bfs_fixed_size_set(expensive_sparse, S{}, 8, max_len));
  TIME(bfs_fixed_size_set(expensive_sparse, S{}, 4, max_len));
  TIME(bfs_fixed_size_set(expensive_sparse, S{}, 2, max_len));
  TIME(bfs_fixed_size_set(expensive_sparse, S{}, 1, max_len));
  std::cout << std::endl;
  // */

}
