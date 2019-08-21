/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file string.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/10/13 19:28:04
 * @brief 
 **/

#include <stdio.h>
#include <string>
#include <stdarg.h>
#include <stddef.h>

#include "util/string.h"

namespace zf {

std::string string_format(const char *msgfmt, ...) {
    std::string str;
    if (msgfmt != NULL) {
        va_list args;
        va_start(args, msgfmt);

        char buf[128];
        vsnprintf(buf, sizeof(buf), msgfmt, args);
        str = buf;

        va_end(args);
    }
    return str;
}

void string_append(std::string *str, const char *msgfmt, ...) {
    if (msgfmt != NULL) {
        va_list args;
        va_start(args, msgfmt);

        char buf[128];
        vsnprintf(buf, sizeof(buf), msgfmt, args);
        *str = buf;

        va_end(args);
    }
}

}

