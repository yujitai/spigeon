/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file engine.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/03 14:42:35
 * @brief 
 **/

#ifndef __STORE_DB_ENGINE_H__
#define __STORE_DB_ENGINE_H__

#include <string>
#include <stdint.h>
#include "util/slice.h"
#include "inc/module.h"

namespace store {
enum {
  ENGINE_OK = 0,
  ENGINE_ERROR = -1,
  ENGINE_NOTFOUND = -2,
  ENGINE_LOCK = -3
};

class Engine : public Module{
public:
    Engine() { }
    virtual ~Engine() { }

    /**
     * @brief get kv
     * @note  memory is allocated inside
     *        and the caller should free the 'value' after using it
     */
    virtual int get(const Slice &key, std::string *value, 
                    uint32_t *expired_time_s = NULL) = 0;

    /**
     * @brief set kv
     */
    virtual int set(const Slice &key, const Slice &value, 
                    uint32_t expired_time_s = 0) = 0;

    /**
     * @brief del kv
     */
    virtual int del(const Slice &key) = 0;

    /**
     * @brief stat
     */
    virtual std::string stat() = 0;
};

} // end of namespace

#endif //__STORE_DB_ENGINE_H__

