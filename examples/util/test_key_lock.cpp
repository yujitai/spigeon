/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file test_key_lock.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/06 18:43:06
 * @brief 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>

#include "util/key_lock.h"

using namespace store;

KeyLock g_key_lock;

time_t get_sys_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void *test1(void *ptr) {
    (void)ptr;

    printf("test1 will lock a...\n");
    g_key_lock.lock("a");
    printf("test1 locked a...\n");
    sleep(3);
    printf("test1 will unlock a...\n");
    g_key_lock.unlock("a");

    printf("test1 will lock b...\n");
    g_key_lock.lock("b");
    printf("test1 locked b...\n");
    sleep(3);
    printf("test1 will unlock b...\n");
    g_key_lock.unlock("b");

    return NULL;
}

void *test2(void *ptr) {
    (void)ptr;

    printf("test2 will try lock a...\n");
    bool try_succeed;
    g_key_lock.try_lock("a", &try_succeed);
    if (try_succeed) {
        printf("test2 try lock succeed: a...\n");
    } else {
        printf("test2 try lock failed: a...\n");
        g_key_lock.lock("a");
        printf("test2 locked a...\n");
    }
    sleep(3);
    printf("test2 will unlock a...\n");
    g_key_lock.unlock("a");

    printf("test2 will lock b...\n");
    g_key_lock.lock("b");
    printf("test2 locked b...\n");
    sleep(3);
    printf("test2 will unlock b...\n");
    g_key_lock.unlock("b");

    return NULL;
}

void *test3(void *ptr) {
    (void)ptr;

    printf("test3 will lock 123...\n");
    g_key_lock.lock("123");
    printf("test3 locked 123...\n");
    sleep(3);
    printf("test3 will unlock 123...\n");
    g_key_lock.unlock("123");

    printf("test3 will lock b...\n");
    g_key_lock.lock("b");
    printf("test3 locked b...\n");
    sleep(3);
    printf("test3 will unlock b...\n");
    g_key_lock.unlock("b");

    return NULL;
}

int main() {

    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;

    pthread_create(&thread1, NULL, test1, NULL);
    pthread_create(&thread2, NULL, test2, NULL);
    pthread_create(&thread3, NULL, test3, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    return 0;
}



