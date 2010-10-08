// Runtime support
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _runtime_Util_h_
#define _runtime_Util_h_

extern "C" {

char* _crack_strerror(void);

void _crack_float_str(double, char* buf, unsigned int size);

}

#endif // _runtime_Util_h_
