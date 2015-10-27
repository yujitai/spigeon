#include "server/thread.h"
namespace store {
void *run(void *arg) {
    Runnable *runnable = (Runnable*)arg;
    runnable->run();
    pthread_exit(NULL);
}

int create_thread(Runnable *runnable) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    int rc = pthread_create(&(runnable->thread_id),
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
    int rc = pthread_join(runnable->thread_id, &status);
    if (rc) {
        return THREAD_ERROR;
    } else {
        return THREAD_OK;
    }
}

}
