#include "bfs.hpp"
#include <cassert>
#include <iostream>
#include <string>

size_t max_len = 0;

struct S {

  std::vector<int> a;

  friend bool operator==(const S &l, const S &r) {
    return l.a == r.a;
  }

  size_t hash() const {
    size_t res = 0;
    for (int bit : a)
      res = res<<1 | bit;
    return res;
  }

  std::vector<S> get_transitions() {
    std::vector<S> transitions;

    if (a.size() < max_len) {
      transitions.push_back({a});
      transitions.back().a.push_back(0);
      transitions.push_back({a});
      transitions.back().a.push_back(1);
    }

    if (a.size() > 0) {
      transitions.push_back({a});
      transitions.back().a.pop_back();

      if (a.size() < max_len) {
        for (int bit0 : {0, 1})
          for (int bit1 : {0, 1}) {
            transitions.push_back({a});
            transitions.back().a.pop_back();
            transitions.back().a.push_back(bit0);
            transitions.back().a.push_back(bit1);
          }
      }
    }

    return transitions;
  }
};

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "usage:\n";
    std::cout << argv[0] << " max_len hash_table_bit_cnt thread_count\n";
    return 0;
  }

  max_len = std::stoi(argv[1]);
  int hash_table_bit_cnt = std::stoi(argv[2]);
  int thread_count = std::stoi(argv[3]);

  parallel_bfs(S{}, thread_count, hash_table_bit_cnt);
}
