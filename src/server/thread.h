#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include "util/queue.h"
#include <string>
namespace store {
enum {
    THREAD_OK = 0,
    THREAD_ERROR = 1
};

class Runnable {
  public:
    Runnable(std::string tn = "") : thread_name(tn) {};
    virtual ~Runnable() {};
    virtual void run() = 0;
    pthread_t thread_id;
    std::string thread_name;
};

int create_thread(Runnable *runnable);
int join_thread(Runnable *runnable);
}
#endif
