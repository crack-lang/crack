// Runtime support
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _runtime_Util_h_
#define _runtime_Util_h_

namespace crack { namespace runtime {

char* strerror(void);

void float_str(double, char* buf, unsigned int size);

}}

#endif // _runtime_Util_h_
