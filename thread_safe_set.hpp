#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

template<class T>
class thread_safe_set {
    
  struct Node {
    T here;
    Node* next = nullptr;
  };

  const int bits;
  std::vector<Node*> have;
  std::vector<std::mutex> muteces;

 public:

  thread_safe_set(int bits_) : bits(bits_), have(1<<bits), muteces(1<<bits) {}

  // return true if already there
  bool check_and_emplace(const T &t) {
    size_t key = t.hash() & ((1<<bits)-1);
    std::lock_guard<std::mutex> lock(muteces[key]);

    Node **nd = &have[key];
    while (*nd) {
      if ((*nd)->here == t)
        return true;
      nd = &(*nd)->next;
    }

    *nd = new Node{t, nullptr};
    return false;
  }
};
