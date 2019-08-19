/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file test_time.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/06 15:43:06
 * @brief 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>

#include "event/event.h"
#include "util/store_define.h"

using namespace store;

enum method_t {
    SYS_TIME = 1,
    LIBEV_TIME = 2,
};

int g_method = 1;
int g_thread_num = 1;
uint64_t g_loop_count = 1000000;

time_t get_sys_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

time_t get_current_time() {
    if (g_method == SYS_TIME) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
    } else {
        return store::EventLoop::current_time();
    }
}

void *test_get_time(void *ptr) {
    (void)ptr;

    uint64_t run_start = get_sys_time();
    for (uint64_t i = 0; i < g_loop_count; i++) {
        get_current_time();
    }
    uint64_t run_end = get_sys_time();

	printf("test: count:%lu, start:%lu, end:%lu, timespan: %lu us, avg: %lu us\n",
        g_loop_count, run_start, run_end, 
        run_end-run_start, (run_end-run_start) / g_loop_count);

    return NULL;
}

void test1() {
    EventLoop el(NULL, true);
    if (g_method == LIBEV_TIME) {
        el.run();
    }

    pthread_t *thread_list = (pthread_t*)malloc(g_thread_num * sizeof(pthread_t));
    for (int i = 0; i < g_thread_num; ++i) {
        ASSERT_EQUAL_0(pthread_create(&thread_list[i], NULL, test_get_time, NULL));
    }

    for (int i = 0; i < g_thread_num; ++i) {
        ASSERT_EQUAL_0(pthread_join(thread_list[i], NULL));
    }

    free(thread_list);
}

void load_opt(int argc, char * argv[])
{
	if (argc <= 1) {
		//print_help();
	}

    char c = '\0';
    while (-1 != (c = getopt(argc, argv, "m:t:n:vh?"))) {
        switch(c) {
        case 'm':
            if (strcmp(optarg, "sys") == 0) {
                g_method = SYS_TIME;
            } else if (strcmp(optarg, "libev") == 0) {
                g_method = LIBEV_TIME;
            } else {
				printf("invalid parameter: method\n");
                exit(-1);
            }
            break;
		case 't':
			g_thread_num = strtoull(optarg, NULL, 10);
			break;
		case 'n':
			g_loop_count = strtoull(optarg, NULL, 10);
			break;
        case 'v':
			break;
        case 'h':
        case '?':
        default:
            //print_help();
            break;
        }
    }

	if (g_method == SYS_TIME) {
        printf("method:%s", "use gettimeofday\n");
    } else if (g_method == LIBEV_TIME) {
		printf("method:%s", "use libev time\n");
	}
}

int main(int argc, char **argv) {

	load_opt(argc, argv);

    test1();

    return 0;
}

