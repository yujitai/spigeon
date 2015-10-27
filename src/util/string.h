/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file string.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/10/13 19:25:19
 * @brief 
 **/

#ifndef __UTIL_STRING_H__
#define __UTIL_STRING_H__

namespace store {

std::string string_format(const char *msgfmt, ...) 
    __attribute__ ((format(printf, 1, 2)));

void string_append(std::string *str, const char *msgfmt, ...)
    __attribute__ ((format(printf, 2, 3)));

}

#endif //__UTIL_STRING_H__

