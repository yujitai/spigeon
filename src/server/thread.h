#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include "util/queue.h"
namespace store {
enum {
    THREAD_OK = 0,
    THREAD_ERROR = 1
};

class Runnable {
  public:
    virtual ~Runnable() {} ;
    virtual void run() = 0;
    pthread_t thread_id;
};

int create_thread(Runnable *runnable);
int join_thread(Runnable *runnable);
}
#endif
