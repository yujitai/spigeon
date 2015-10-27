#include <stdio.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "sys/time.h"

#include "util/stat.h"

#include "event/event.h"

using namespace store;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class test_Stat : public ::testing::Test {
protected:
    test_Stat() { }

    virtual ~test_Stat() { }

    virtual void SetUp() { }

    virtual void TearDown() { }

};

int g_thread_num = 100;
int g_loop_count = 10000;

void *test_inc_routine(void *ptr) {
    Stat *stat = (Stat*)ptr;
    for (int i = 0; i < g_loop_count; ++i) {
        stat->inc_cmds("GET");
    }
    return NULL;
}

TEST_F(test_Stat, case_inc) {

    Stat stat;

    pthread_t *thread_list = (pthread_t*)malloc(g_thread_num * sizeof(pthread_t));
    for (int i = 0; i < g_thread_num; ++i) {
        ASSERT_EQ(0, pthread_create(&thread_list[i], NULL, test_inc_routine, &stat));
    }

    for (int i = 0; i < g_thread_num; ++i) {
        ASSERT_EQ(0, pthread_join(thread_list[i], NULL));
    }

    std::list<StatItem> result;
    stat.fetch_all(&result);

    for (std::list<StatItem>::iterator it = result.begin();
        it != result.end();
        ++it)
    {
        printf("debug: k:%s, v:%s\n", it->key.c_str(), it->value.c_str());
        if (it->key == "cmds_get") {
            char expected[128];
            snprintf(expected, sizeof(expected),
                "%lu", (uint64_t)g_thread_num * g_loop_count);
            EXPECT_STREQ(expected, it->value.c_str());
        }
    }

    free(thread_list);
}

//-----------------------

time_t get_current_time() {
#ifdef USE_SYS_TIME
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
#else
    return store::EventLoop::current_time();
#endif
}

void *test_get_time(void *ptr) {
    (void)ptr;

    int loop_count = 100000;

    uint64_t run_start = get_current_time();
    for (int i = 0; i < loop_count; i++) {
        get_current_time();
    }
    uint64_t run_end = get_current_time();

	printf("test: run start:%lu, end:%lu, timespan: %lu us, avg: %lu us\n",
        run_start, run_end, 
        run_end-run_start, (run_end-run_start) / loop_count);

    return NULL;
}

TEST_F(test_Stat, case_get_time){
#ifdef USE_SYS_TIME
    printf("use gettimeofday\n");
#else
    printf("use EventLoop::current_time\n");
    EventLoop el(NULL, true);
    el.run();
#endif

    pthread_t *thread_list = (pthread_t*)malloc(g_thread_num * sizeof(pthread_t));
    for (int i = 0; i < g_thread_num; ++i) {
        ASSERT_EQ(0, pthread_create(&thread_list[i], NULL, test_get_time, NULL));
    }

    for (int i = 0; i < g_thread_num; ++i) {
        ASSERT_EQ(0, pthread_join(thread_list[i], NULL));
    }

    free(thread_list);
}

