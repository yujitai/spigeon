/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file url_snprintf.h
 * @author liuqingjun(com@baidu.com)
 * @date 2012/11/23 11:34:22
 * @brief 
 *  
 **/




#ifndef  __URL_SNPRINTF_H_
#define  __URL_SNPRINTF_H_

#include <stdarg.h>
#include <stddef.h>
int url_vsnprintf(char *original_buf, size_t size, const char *fmt, va_list vl);
int url_snprintf(char *original_buf, size_t size, const char *fmt, ...);

#endif  //__URL_SNPRINTF_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
