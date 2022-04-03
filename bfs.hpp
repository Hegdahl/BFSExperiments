#pragma once

#include "chunked_vector.hpp"
#include "thread_safe_set.hpp"

#include <iostream>
#include <thread>
#include <unordered_set>
#include <utility>

template<class T>
void parallel_bfs_step(
    thread_safe_set<T> *vis,
    typename chunked_vector<T>::iterator begin,
    typename chunked_vector<T>::iterator end,
    std::vector<T> *new_q) {
  for (; begin != end; ++begin) {
    T &current = *begin;
    for (T &next : current.get_transitions()) {
      if (vis->check_and_emplace(next))
        continue;
      new_q->push_back(next);
    }
  }
}

template<class T>
void parallel_bfs(const T &initial_state, int worker_cnt) {
  chunked_vector<T> q0(worker_cnt), q1(worker_cnt);
  q0.chunks[0].push_back(initial_state);

  thread_safe_set<T> vis;
  vis.check_and_emplace(initial_state);

  std::vector<std::thread> workers(worker_cnt);

  size_t vis_cnt = 0;
  size_t q_size;
  while ((q_size = q0.size())) {
    vis_cnt += q_size;
    size_t per_worker = (q_size + worker_cnt - 1) / worker_cnt;

    for (int worker_id = 0; worker_id < worker_cnt; ++worker_id) {
      size_t begin = std::min(per_worker * worker_id, q_size);
      size_t end = std::min(per_worker * (worker_id + 1), q_size);

      workers[worker_id] = std::thread(
        parallel_bfs_step<T>,
        &vis,
        q0.begin() + begin,
        q0.begin() + end,
        &q1.chunks[worker_id]
      );
    }

    for (std::thread &worker : workers)
      worker.join();

    std::swap(q0, q1);
    q1.clear();
  }

  std::cerr << "total #visits: " << vis_cnt << '\n';
}

struct hasher {
  template<class T>
  auto operator()(const T &x) const {
    return x.hash();
  }
};

template<class T>
void concurrent_bfs(const T &initial_state) {
  std::vector<T> q0 {initial_state}, q1;

  std::unordered_set<T, hasher> vis;
  vis.insert(initial_state);

  size_t vis_cnt = 0;
  while (q0.size()) {
    vis_cnt += q0.size();

    for (T &current : q0) {
      for (T &next : current.get_transitions()) {
        if (!vis.emplace(next).second)
          continue;
        q1.push_back(std::move(next));
      }
    }

    std::swap(q0, q1);
    q1.clear();
  }

  std::cerr << "total #visits: " << vis_cnt << '\n';
}
