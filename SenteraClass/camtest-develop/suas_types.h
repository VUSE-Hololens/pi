/*
 * types.h
 *
 *  Created on: Dec 20, 2011
 *      Author: Andrew Muehlfeld
 */

#ifndef SUAS_TYPES_H_
#define SUAS_TYPES_H_

#include <sys/types.h>

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__))
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
#else
typedef u_int8_t  uint8;
typedef u_int16_t uint16;
typedef u_int32_t uint32;
typedef u_int64_t uint64;
#endif
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;


#endif /* SUAS_TYPES_H_ */
