
#ifndef __UTIL_ATOMIC_H__
#define __UTIL_ATOMIC_H__

/* @auhtor li zhe */

#include <stdint.h>

int atomic_add(int *value, int incr);
uint32_t atomic_add(uint32_t *value, uint32_t incr);
int64_t atomic_add(int64_t *value, int64_t incr);
uint64_t atomic_add(uint64_t *value, uint64_t incr);

int atomic_sub(int *value, int incr);
uint32_t atomic_sub(uint32_t *value, uint32_t incr);
int64_t atomic_sub(int64_t *value, int64_t incr);
uint64_t atomic_sub(uint64_t *value, uint64_t incr);

#endif

