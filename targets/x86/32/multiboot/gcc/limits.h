#ifndef _LIMITS_H_
#define _LIMITS_H_

// Just a quick and dirty limits.h to get things to compile.
// Apparently it is not always possible to just include the limits.h file that came with the compiler.
//
// This is for 32-bit gcc.

#define CHAR_BIT 8
#define SCAR_MIN -128
#define SCHAR_MAX 127
#define UCHAR_MAX 255

#define INT_MAX  2147483647
#define INT_MIN  −2147483648

#define SHRT_MAX 32767
#define SHRT_MIN -32768

#define LONG_MIN INT_MIN
#define LONG_MAX INT_MAX

#define USHRT_MAX 0xFFFF
#define UINT_MAX 0xFFFFFFFF
#define ULONG_MAX UINT_MAX

#endif

