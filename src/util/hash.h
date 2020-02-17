/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file hash.h
 * @author yujitai@zuoyebang.com
 * @date 2020/02/17 11:54:00
 * @brief 
 *  
 **/


#ifndef  __HASH_H_
#define  __HASH_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace zf {

// MurmurHash2, 64-bit versions, by Austin Appleby
// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
// and endian-ness issues if used across multiple platforms.
// 64-bit hash for 64-bit platforms
uint64_t MurmurHash64A (const void * key, int len, unsigned int seed);

typedef unsigned long hash_t;
hash_t hash(const char* src, size_t size);

hash_t strhash(const char* s);

} // namespace zf

#endif  //__HASH_H_


