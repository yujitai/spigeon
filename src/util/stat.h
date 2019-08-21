/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file stat.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/03 20:32:13
 * @brief 
 **/

#ifndef __STORE_UTIL_STAT_H__
#define __STORE_UTIL_STAT_H__

#include <stdint.h>
#include <string>
#include <map>

#include "util/lock.h"

namespace zf {

/**
 * 统计时按线程维度记录数据，查询时可以做sum/max的计算
 */

//destroy
void stat_destroy();
// set
void stat_set(const char *key, int64_t value);
// 仅当大于时才set
void stat_set_max(const char *key, int64_t value);

// 递增
void stat_incr(const std::string &key, int step = 1);
// 递减
void stat_decr(const char *key, int step = 1);

typedef std::map<std::string, int64_t> stat_container_t;
typedef std::map<std::string, int64_t>::iterator stat_container_iter_t;
// 获取统计信息
// sum_result是各个线程的统计累加值，max_result是各个线程里统计的最大值
// 一般情况下，sum或者max二者中只有一个是符合应用预期的，应用自行选择即可
// 如：total_connections从sum_result里获取，max_reply_size是从max_result里获取
void stat_get_all(stat_container_t *sum_result, stat_container_t *max_result);

// 获取当前线程的统计值
void stat_get(const char *key, int64_t *value); // if key not exist, value will 0

// 不推荐使用以下接口，当大量调用时，锁的开销很大
void stat_get_sum(const char *key, int64_t *value);
void stat_get_max(const char *key, int64_t *value);

} // namespace zf

#endif //__STORE_UTIL_STAT_H__


