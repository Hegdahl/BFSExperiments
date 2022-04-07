#pragma once

#include "chunked_vector.hpp"
#include "thread_safe_set.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <utility>

namespace bfs_detail {

template<class T>
struct deref_hash {
  decltype(auto) operator()(const T *x) const {
    return std::hash<T>{}(*x);
  }
};

template<class T>
struct deref_equal {
  decltype(auto) operator()(const T *l, const T *r) const {
    return *l == *r;
  }
};

template<class T>
void parallel_bfs_step(
    thread_safe_set<const T*, deref_hash<T>, deref_equal<T>> *vis,
    //thread_safe_set<T> *vis,
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

template<class T>
void bfs(const T &initial_state, int worker_cnt, int hash_table_bit_cnt) {

  std::vector<chunked_vector<std::unique_ptr<T>>> layers;
  layers.emplace_back(worker_cnt);
  layers.back().chunk(0).push_back(std::make_unique<T>(initial_state));

  thread_safe_set<const T*, bfs_detail::deref_hash<T>, bfs_detail::deref_equal<T>> vis(hash_table_bit_cnt);
  vis.check_and_emplace((*layers.back().begin()).get());

  std::vector<std::thread> workers(worker_cnt);

  size_t vis_cnt = 0;
  size_t q_size;
  while ((q_size = layers.back().size())) {
    std::cerr << "step size: " << q_size << '\n';

    layers.emplace_back(worker_cnt);

    vis_cnt += q_size;

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
