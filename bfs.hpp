#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "chunked_vector.hpp"
#include "fixed_size_set.hpp"
#include "parallel_hashmap/phmap.h"

template <class Neighbors, class T, class Hash = std::hash<T>,
          class KeyEqual = std::equal_to<T>>
bool sequential_bfs(Neighbors &&neighbors, const T &initial_state) {
  std::vector<std::vector<T>> layers;
  layers.emplace_back();
  layers.back().push_back(initial_state);

  phmap::parallel_flat_hash_set<T, Hash, KeyEqual> vis;
  vis.emplace(initial_state);

  auto step = [&]() {
    for (auto &node : layers.end()[-2])
      for (auto next : neighbors(node))
        if (vis.emplace(next).second) layers.back().push_back(next);
  };

  std::size_t q_size = layers.back().size();
  while ((q_size = layers.back().size())) {
    layers.emplace_back();
    step();
  }

  return false;
}

template <class Neighbors, class T, class VisSet>
bool bfs(Neighbors &&neighbors, const T &initial_state, int thread_count, VisSet &&vis) {
  std::vector<chunked_vector<T>> layers;
  layers.emplace_back(thread_count);
  layers.back().chunk(0).push_back(initial_state);

  vis.emplace(initial_state);

  std::vector<std::thread> threads(thread_count);

  auto step = [&](int thread_id, std::size_t begin_index,
                  std::size_t end_index) {
    auto &new_queue = layers.back().chunk(thread_id);
    auto begin = layers.end()[-2].begin() + begin_index;
    auto end = layers.end()[-2].begin() + end_index;
    while (begin != end)
      for (auto next : neighbors(*(begin++)))
        if (vis.emplace(next).second) new_queue.push_back(next);
  };

  std::size_t q_size = layers.back().size();
  while ((q_size = layers.back().size())) {
    layers.emplace_back(thread_count);

    size_t per_thread = (q_size + thread_count - 1) / thread_count;

    for (int thread_id = 0; thread_id < thread_count; ++thread_id) {
      size_t begin = std::min(per_thread * thread_id, q_size);
      size_t end = std::min(per_thread * (thread_id + 1), q_size);
      threads[thread_id] = std::thread(step, thread_id, begin, end);
    }

    for (auto &thread : threads) thread.join();
  }

  return false;
}

template <class Neighbors, class T, class Hash = std::hash<T>,
          class KeyEqual = std::equal_to<T>>
bool bfs_phmap(Neighbors &&neighbors, const T &initial_state,
               int thread_count) {
  phmap::parallel_flat_hash_set<T, Hash, KeyEqual, phmap::priv::Allocator<T>,
                                4UL, std::mutex>
      vis;
  return bfs(std::forward<Neighbors>(neighbors), initial_state, thread_count, vis);
}

template <class Neighbors, class T, class Hash = std::hash<T>,
          class KeyEqual = std::equal_to<T>>
bool bfs_fixed_size_set(Neighbors &&neighbors, const T &initial_state,
                        int thread_count, int hash_bit_count) {
  fixed_size_set<T, Hash, KeyEqual> vis(hash_bit_count);
  return bfs(std::forward<Neighbors>(neighbors), initial_state, thread_count, vis);
}
