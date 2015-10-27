#define USE_EV_TIME true

#include <stdio.h>
#include <unistd.h>
#include "util/log.h"
#include "event/event.h"
#include "gtest/gtest.h"

using namespace store;

/**
 * @brief 测试Log库
 */
class test_Log_suite : public ::testing::Test {
protected:
    test_Log_suite() { }

    virtual ~test_Log_suite() { }

    virtual void SetUp() { }

    virtual void TearDown() { }

};

static log_data_t* get_log_data_1() {
    log_data_t* log_data = log_data_new();
    log_data_push(log_data, "log_id", "1");
    log_data_push(log_data, "string key", "string value");
    log_data_push_int(log_data, "int key", 20);
    log_data_push_uint(log_data, "uint key", 100);
    log_data_push_double(log_data, "double key", 3.141592653);
    return log_data;
}

static log_data_t* get_log_data_2() {
    log_data_t* log_data = log_data_new();
    log_data_push(log_data, "log_id", "2");
    log_data_push(log_data, "string key", "string value");
    log_data_push_int(log_data, "int key", 20);
    log_data_push_uint(log_data, "uint key", 100);
    log_data_push_double(log_data, "double key", 3.141592653);
    return log_data;
}

static void* thread_loop(void* id) {
    //printf("enter %s\n", __FUNCTION__);
    int thread_id = (int)(intptr_t)id;
    log_data_t* log_data = get_log_data_1();
    if (0 == thread_id) {
        rlog_warning(log_data, "thread %d", thread_id);
    } else {
        rlog_debug(log_data, "thread %d", thread_id);
    }
    log_debug("no thread data");
    set_thread_log_data(get_log_data_1());
    log_debug("thread log_id 1");
    set_thread_log_data(get_log_data_2());
    log_debug("thread log_id 2");
    set_thread_log_data(get_log_data_1());
    log_debug("thread log_id 1");
    //log_data_free(log_data);
    //log_data = NULL;
    //printf("leave %s\n", __FUNCTION__);
    return NULL;
}

TEST_F(test_Log_suite, create_not_existing_product) {
    const char* dir = ".";
    const char* bin_name = "Store";
    int log_level = 16;  // LOG_TRACE

    log_init(dir, bin_name, log_level);

    const int thread_num = 5;
    pthread_t threads[thread_num];

    for (int i = 0; i < thread_num; ++i) {
        int ret = pthread_create(&threads[i], NULL, thread_loop, (void*)(intptr_t)i);
        if (ret) {
            perror("thread create failed");
            return;
        }
    }
    sleep(1);
}

int main(int argc, char **argv) {
    //EventLoop* el = new EventLoop(NULL, true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

