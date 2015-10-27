/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file store_define.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/26 17:25:07
 * @brief 全局常用的宏放在这里
 **/

#ifndef __STORE_DEFINE_H__
#define __STORE_DEFINE_H__

// 全局常用的宏放在这里

#define UNUSED(x) (void)(x)

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "log.h"

#ifdef DEBUG
#define ABORT() abort()
#else // not DEBUG
#define ABORT() raise(SIGKILL)
#endif // end DEBUG

#define ASSERT(exp) \
do { \
    if (!(exp)) { \
        log_fatal("assert failed: %s", #exp); \
        usleep(100*1000); /*waiting for log thread*/ \
        ABORT(); \
    } \
} while (0) \

#define ASSERT_EQUAL(actual, expect) \
do { \
    int a = (actual); \
    int e = (expect); \
    if (a != e) { \
        log_fatal("assert equal failed: %s, actual:%d, expect:%d", #actual, a, e); \
        usleep(100*1000); /*waiting for log thread*/ \
        ABORT(); \
    } \
} while (0) \

#define ASSERT_EQUAL_0(exp) ASSERT_EQUAL(exp, 0)

#define TIME_US_DIFF(pre, cur) (((cur.tv_sec)-(pre.tv_sec))*1000000 + (cur.tv_usec) - (pre.tv_usec))

#endif //__STORE_DEFINE_H__

