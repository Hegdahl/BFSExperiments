#include <atomic>
#include <cstddef>
#include <utility>

template<class T>
class thread_safe_set {
  struct Node {
    T here;
    std::atomic<Node*> next = nullptr;
  };

  std::atomic<Node*> have[1<<28]{};

 public:

  // return true if already there
  bool check_and_emplace(const T &t) {
    std::atomic<Node*> &nd = find(t);
    if (nd) return true;
    nd = new Node{t, nullptr};
    return false;
  }

  std::atomic<Node*> &find(const T &t) {
    std::atomic<Node*> *nd = have + t.hash() % (1 << 25);
    while (*nd && (**nd).here != t)
      nd = &(**nd).next;
    return *nd;
  }
};
