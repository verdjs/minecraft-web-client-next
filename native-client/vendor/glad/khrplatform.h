#ifndef __khrplatform_h_
#define __khrplatform_h_

#if defined(_WIN32)
#define KHRONOS_APICALL __declspec(dllimport)
#define KHRONOS_APIENTRY __stdcall
#else
#define KHRONOS_APICALL
#define KHRONOS_APIENTRY
#endif

#include <stdint.h>
typedef int32_t                 khronos_int32_t;
typedef uint32_t                khronos_uint32_t;
typedef int64_t                 khronos_int64_t;
typedef uint64_t                khronos_uint64_t;
typedef float                   khronos_float_t;

#endif
