#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <string>

#include "util/queue.h"

namespace zf {

enum {
    THREAD_OK = 0,
    THREAD_ERROR = 1
};

class Runnable {
public:
    Runnable(const std::string& thread_name = "") 
        : _thread_name(thread_name) {}
    virtual ~Runnable() {};

    virtual void run() = 0;

    pthread_t _thread_id;
    std::string _thread_name;
};

int create_thread(Runnable *runnable);
int join_thread(Runnable *runnable);

} // namespace zf

#endif


