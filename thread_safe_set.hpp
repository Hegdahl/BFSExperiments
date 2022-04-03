#include <cstddef>
#include <mutex>
#include <utility>

template<class T>
class thread_safe_set {
  static constexpr size_t sz = 1<<24;
    
  struct Node {
    T here;
    Node* next = nullptr;
  };

  Node* have[sz]{};
  std::mutex muteces[sz];

 public:

  // return true if already there
  bool check_and_emplace(const T &t) {
    size_t key = t.hash() % sz;
    std::lock_guard<std::mutex> lock(muteces[key]);

    Node **nd = have + key;
    while (*nd) {
      if ((*nd)->here == t)
        return true;
      nd = &(*nd)->next;
    }

    *nd = new Node{t, nullptr};
    return false;
  }
};
