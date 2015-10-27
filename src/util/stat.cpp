/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file stat.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/03 23:03:38
 * @brief 
 **/

#include <pthread.h>
#include "stat.h"
#include <map>
#include <vector>

#include "util/store_define.h"

namespace store {

typedef struct {
    Mutex mutex; // lock before aggregate or update
    stat_container_t int_value;
} stat_data_t;

static pthread_once_t stat_pthread_once = PTHREAD_ONCE_INIT;
static pthread_key_t stat_pthread_key;
static Mutex stat_thread_data_list_mutex; // only used to protect stat_thread_data_list
static std::vector<stat_data_t*> stat_thread_data_list;

static void stat_create_pthread_key() {
    ASSERT_EQUAL_0(pthread_key_create(&stat_pthread_key, NULL));
}

static stat_data_t *stat_init_thread_data() {
    ASSERT(NULL == pthread_getspecific(stat_pthread_key));
    MutexHolder holder(stat_thread_data_list_mutex);
    stat_data_t *data = new stat_data_t;
    ASSERT(data);
    ASSERT_EQUAL_0(pthread_setspecific(stat_pthread_key, data));
    stat_thread_data_list.push_back(data);
    return data;
}

// stat_close? TODO

static inline stat_data_t *stat_get_thread_data() { 
    ASSERT_EQUAL_0(pthread_once(&stat_pthread_once, stat_create_pthread_key)); 

    void *d = pthread_getspecific(stat_pthread_key); 
    if (d) {
        return (stat_data_t*)d;
    }
    return stat_init_thread_data();
}

void stat_set(const char *key, int64_t value) {
    stat_data_t *data = stat_get_thread_data();
    MutexHolder holder(data->mutex);
    data->int_value[key] = value;
}

void stat_set_max(const char *key, int64_t value) {
    stat_data_t *data = stat_get_thread_data();
    MutexHolder holder(data->mutex);
    if (value > data->int_value[key]) {
        data->int_value[key] = value;
    }
}

void stat_incr(const std::string &key, int step) {
    stat_data_t *data = stat_get_thread_data();
    MutexHolder holder(data->mutex);
    data->int_value[key] += step;
}

void stat_decr(const char *key, int step) {
    stat_data_t *data = stat_get_thread_data();
    MutexHolder holder(data->mutex);
    data->int_value[key] -= step;
}

void stat_get(const char *key, int64_t *value) {
    stat_data_t *data = stat_get_thread_data();
    MutexHolder holder(data->mutex);
    *value = data->int_value[key];
}

void stat_get_all(stat_container_t *sum_result, stat_container_t *max_result) {
    sum_result->clear();
    max_result->clear();

    for (std::vector<stat_data_t*>::iterator data_iter = stat_thread_data_list.begin();
        data_iter != stat_thread_data_list.end();
        ++data_iter)
    {
        stat_data_t *data = *data_iter;
        MutexHolder holder(data->mutex);

        for (stat_container_iter_t iter = data->int_value.begin();
            iter != data->int_value.end();
            iter++)
        {
            (*sum_result)[iter->first] += iter->second;

            if (max_result->find(iter->first) == max_result->end()) {
                (*max_result)[iter->first] = iter->second;
            } if ((*max_result)[iter->first] < iter->second) {
                (*max_result)[iter->first] = iter->second;
            }
        }
    }
}

void stat_get_sum(const char *key, int64_t *value) {
    int64_t result = 0;
    for (std::vector<stat_data_t*>::iterator data_iter = stat_thread_data_list.begin();
        data_iter != stat_thread_data_list.end();
        ++data_iter)
    {
        stat_data_t *data = *data_iter;
        MutexHolder holder(data->mutex);
        result += data->int_value[key];
    }
    *value = result;
}

void stat_get_max(const char *key, int64_t *value) {
    int64_t result = INT64_MIN;
    stat_container_iter_t iter;
    for (std::vector<stat_data_t*>::iterator data_iter = stat_thread_data_list.begin();
        data_iter != stat_thread_data_list.end();
        ++data_iter)
    {
        stat_data_t *data = *data_iter;
        MutexHolder holder(data->mutex);
        iter = data->int_value.find(key);
        if (iter != data->int_value.end() && iter->second > result) {
            result = iter->second;
        }
    }
    *value = result;
}

}

