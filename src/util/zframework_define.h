/*****************************************************************
* Copyright (C) 2020 Zuoyebang.com, Inc. All Rights Reserved.
* 
* @file zframework_define.h
* @author yujitai(yujitai@zuoyebang.com)
* @date 2020/03/23
* @brief 
*****************************************************************/


#ifndef _ZFRAMEWORK_DEFINE_H
#define _ZFRAMEWORK_DEFINE_H

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "log.h"

#define UNUSED(x) (void)(x)

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

#endif // _ZFRAMEWORK_DEFINE_H


