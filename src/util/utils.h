#ifndef _UTILS_H_
#define _UTILS_H_
#include <cstring>

long long timetoll(const char *p, int *err);
long long memtoll(const char *p, int *err);

int ll2string(char *s, size_t len, long long value);

int string2ll(const char *s, size_t slen, long long *value);

int string2l(const char *s, size_t slen, long *lval);

int d2string(char *buf, size_t len, double value);

void getRandomHexChars(char *p, unsigned int len);

long long ustime(void);

long long mstime(void);
#endif
