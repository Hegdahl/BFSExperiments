#pragma once

#include "chunked_vector.hpp"
#include "thread_safe_set.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <utility>

/**
 * Namespace containing implementation
 * details for the parallel BFS.
 */
namespace bfs_detail {

/**
 * Function object for getting
 * the hash of an object from a
 * pointer to the object.
 *
 * @tparam T the type to hash
 */
template<class T>
struct deref_hash {
  decltype(auto) operator()(const T *x) const {
    return std::hash<T>{}(*x);
  }
};

/**
 * Function object for comparing
 * objects from pointers to the objects.
 *
 * @tparam T the type to compare
 */
template<class T>
struct deref_equal {
  decltype(auto) operator()(const T *l, const T *r) const {
    return *l == *r;
  }
};

/**
 * Runs a single thread's work in a single step in a BFS.
 *
 * @tparam T    the type of the nodes
 * @param vis   set of already visited nodes
 * @param begin iterator pointing to the first node
 *              this thread is responsible for visiting
 * @param end   iterator pointing past the last node
 *              this thread is responsible for visiting
 * @param end   location to store unvisited vertices
 *              reachable in this step
 */
template<class T>
void parallel_bfs_step(
    thread_safe_set<const T*, deref_hash<T>, deref_equal<T>> *vis,
    typename chunked_vector<std::unique_ptr<T>>::iterator begin,
    typename chunked_vector<std::unique_ptr<T>>::iterator end,
    std::vector<std::unique_ptr<T>> *new_q) {
  for (; begin != end; ++begin) {
    T *current = (*begin).get();
    for (std::unique_ptr<T> &next : current->get_transitions()) {
      if (vis->check_and_emplace(next.get()))
        continue;
      new_q->push_back(std::move(next));
    }
  }
}

} // namespace bfs_detail

/**
 * Runs a breath first search sped up by parallelism.
 *
 * Assuming std::hash<T>::operator() is defined and is a reasonable hash function.
 *
 * time:   O(V + E * (1 + V / (1<<hash_table_bit_cnt)))
 * memory: O(V + (1<<hash_table_bit_cnt))
 *
 * If 1<<hash_table_bit_cnt ~ V this simplifies to
 *
 * time:   O(V + E)
 * memory: O(V)
 *
 * @tparam T 
 * @param initial_state      the node to start from
 * @param worker_cnt         how many threads to use (excluding the master thread)
 * @param hash_table_bit_cnt how many bits to use in the hash table indices.
 *                           number of buckets will be 1<<hash_table_bit_cnt
 */
template<class T>
void bfs(const T &initial_state, int worker_cnt, int hash_table_bit_cnt) {

  // Create a list of layers and inserst the initial state.
  // Each layer has to be stored since
  // the visited set only stores pointers
  // to the nodes stored in the layer.
  std::vector<chunked_vector<std::unique_ptr<T>>> layers;
  layers.emplace_back(worker_cnt);
  layers.back().chunk(0).push_back(std::make_unique<T>(initial_state));

  // Initialize the set of visited nodes
  // with only the initial state marked as visited.
  thread_safe_set<const T*, bfs_detail::deref_hash<T>, bfs_detail::deref_equal<T>>
    vis(hash_table_bit_cnt);
  vis.check_and_emplace((*layers.back().begin()).get());

  std::vector<std::thread> workers(worker_cnt);

  size_t vis_cnt = 0;

  size_t q_size;
  while ((q_size = layers.back().size())) {
    std::cerr << "step size: " << q_size << '\n';

    layers.emplace_back(worker_cnt);

    vis_cnt += q_size;

    // number of nodes each worker should visit (rounded up)
    size_t per_worker = (q_size + worker_cnt - 1) / worker_cnt;

    for (int worker_id = 0; worker_id < worker_cnt; ++worker_id) {
      size_t begin = std::min(per_worker * worker_id, q_size);
      size_t end = std::min(per_worker * (worker_id + 1), q_size);

      workers[worker_id] = std::thread(
          bfs_detail::parallel_bfs_step<T>,
          &vis,
          layers.end()[-2].begin() + begin,
          layers.end()[-2].begin() + end,
          &layers.end()[-1].chunk(worker_id)
      );
    }

    for (std::thread &worker : workers)
      worker.join();
  }

  std::cerr << "total #visits: " << vis_cnt << '\n';
}
