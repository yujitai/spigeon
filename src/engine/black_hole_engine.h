/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file black_hole_engine.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/03 15:40:19
 * @brief 
 **/

#ifndef __STORE_DB_BLACK_HOLE_ENGINE_H__
#define __STORE_DB_BLACK_HOLE_ENGINE_H__

#include <string>
#include "engine/engine.h"
#include "util/zmalloc.h"
#include "util/store_define.h"

namespace store {

/**
 * @brief Black Hole engine, just for testing
 */
class BlackHoleEngine : public Engine {
public:
    BlackHoleEngine() {}
    virtual ~BlackHoleEngine() {}

    virtual int get(const Slice &key, std::string *value,
                    uint32_t *expired_time_s = NULL) {
        *value = std::string(key.data(), key.size());
        return 0;
    }

    virtual int set(const Slice &key, const Slice &value,
                    uint32_t expired_time_s = 0) {
        UNUSED(key);
        UNUSED(value);
        UNUSED(expired_time_s);
        return 0;
    }

    virtual int del(const Slice &key) {
        UNUSED(key);
        return 0;
    }

    virtual std::string stat() {
        return "";
    }
};

} // end of namespace

#endif //__STORE_DB_BLACK_HOLE_ENGINE_H__

