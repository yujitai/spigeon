#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#include "atomic.h"

#define INC_TO 1000000

uint64_t g_int64_begin = 1000UL * 1000 * 1000 * 1000;

int g_int32 = 0;
uint32_t g_uint32 = 0;
int64_t g_int64 = g_int64_begin;
uint64_t g_uint64 = g_int64_begin;

pid_t gettid(void) {
    return syscall(__NR_gettid);
}

void *thread_routine(void *arg) {
    int i;
    int proc_num = (int)(long)arg;
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(proc_num, &set);

    if (sched_setaffinity(gettid(), sizeof(cpu_set_t), &set)) {
        perror("sched_setaffinity");
        return NULL;
    }

    for (i = 0; i < INC_TO; i++) {
        //g_int32++;
        atomic_add(&g_int32, 1);
        atomic_add(&g_uint32, 1);
        atomic_add(&g_int64, 1);
        atomic_add(&g_uint64, 1);
    }

    return NULL;
}

int main() {
    int procs = 0;
    int i;
    pthread_t *thrs;

    // Getting number of CPUs
    procs = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (procs < 0) {
        perror("sysconf");
        return -1;
    }

    thrs = (pthread_t *)malloc(sizeof(pthread_t) * procs);
    if (thrs == NULL) {
        perror("malloc");
        return -1;
    }

    printf("Starting %d threads...\n", procs);

    for (i = 0; i < procs; i++) {
        if (pthread_create(&thrs[i], NULL, thread_routine,
                (void *)(long)i)) {
            perror("pthread_create");
            procs = i;
            break;
        }
    }

    for (i = 0; i < procs; i++) {
        pthread_join(thrs[i], NULL);
    }

    free(thrs);

    printf("After doing all the math, g_int32 value is: %d\n", g_int32);
    printf("Expected value is: %d\n", INC_TO * procs);

    printf("After doing all the math, g_uint32 value is: %d\n", g_uint32);
    printf("Expected value is: %d\n", INC_TO * procs);

    printf("After doing all the math, g_int64 value is: %lu\n", g_int64);
    printf("Expected value is: %lu\n", g_int64_begin + INC_TO * procs);

    printf("After doing all the math, g_uint64 value is: %lu\n", g_uint64);
    printf("Expected value is: %lu\n", g_int64_begin + INC_TO * procs);

    return 0;
}
