#include <stdint.h>

#if !defined(MYDEF_H)

#define private_func static
#define local_persist static
#define global_var static

typedef double float64;
typedef float float32;
typedef int32_t bool32;
typedef char int4_t;

#define PI32 3.14159274101257324f

#define MYDEF_H
#endif
