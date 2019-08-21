#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <cstddef>

// a single producer and single consumer queue
template <typename T>
class LockFreeQueue {
private:
  struct Node {
    Node(T val) : value(val), next(NULL) {}
    T value;
    Node * next;
  };
  Node *first;                  // for producer only
  // pointer read and write operation should be atomic on x86-64 platform
  Node *divider, *last;         // shared
public:
  LockFreeQueue() {
    first = divider = last = new Node(T());
  }

  ~LockFreeQueue() {
    while (first != NULL) {
      Node *temp = first;
      first = first->next;
      delete temp;
    }
  }

  void produce(const T& t) {
    last->next = new Node(t);
    last = last->next;
    while (first != divider) {
      Node *temp = first;
      first = first->next;
      delete temp;
    }
  }

  bool consume(T* result) {
    if (divider != last) {
      *result = divider->next->value;
      divider = divider->next;
      return true;
    }
    return false;
  }
};

#endif


