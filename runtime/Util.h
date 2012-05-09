// Runtime support
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _runtime_Util_h_
#define _runtime_Util_h_

#include <stdint.h>

namespace crack { namespace runtime {

char* strerror(void);

int float_str(double, char* buf, unsigned int size);
unsigned int rand(unsigned int low, unsigned int high);
void puts(char *str);
void __putc(char byte);
void __die(const char *message);
void printfloat(float val);
void printint(int val);
void printint64(int64_t val);
void printuint64(uint64_t val);

int is_file(const char *path);
bool fileExists(const char *path);
int setNonBlocking(int fd, int val);
int setUtimes(const char *path, int64_t atv_usecs, int64_t mtv_usecs, bool now);
}}

#endif // _runtime_Util_h_
