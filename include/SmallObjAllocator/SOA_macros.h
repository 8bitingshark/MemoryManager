#ifndef SOA_MACROS_H
#define SOA_MACROS_H

#include "SOA_defines.h"

#define SOA_MALLOC(n)        soa::soa_malloc(n)
#define SOA_FREE(p, n)       soa::soa_free((p), (n))

#define SOA_NEW(T, ...)      soa::soa_new<T>(__VA_ARGS__)
#define SOA_DELETE(T, p)     soa::soa_delete<T>((p))

#define SOA_NEW_A(T, count, ...)   soa::soa_new_array<T>((count), __VA_ARGS__)
#define SOA_DELETE_A(T, p)         soa::soa_delete_array<T>((p))

#endif // SOA_MACROS_H