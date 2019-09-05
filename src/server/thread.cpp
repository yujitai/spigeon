#include "server/thread.h"

namespace zf {

void *run(void *arg) {
    Runnable *runnable = (Runnable*)arg;
    if (runnable->_thread_name != "") {
      // prctl(PR_SET_NAME, runnable->thread_name.c_str());
      pthread_setname_np(pthread_self(), runnable->_thread_name.c_str());
    }
    runnable->run();
    pthread_exit(NULL);
}

int create_thread(Runnable *runnable) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int rc = pthread_create(&(runnable->_thread_id),
                            &attr,
                            run,
                            (void*)runnable);
    pthread_attr_destroy(&attr);
    if (rc) {
        return THREAD_ERROR;
    } else {
        return THREAD_OK;
    }
}

int join_thread(Runnable *runnable) {
    void *status;
    int rc = pthread_join(runnable->_thread_id, &status);
    if (rc) {
        return THREAD_ERROR;
    } else {
        return THREAD_OK;
    }
}

} // namespace zf


