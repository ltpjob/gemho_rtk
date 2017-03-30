/*
 * bsp_debug.h
 *
 *  Created on: 2015-1-6
 *      Author: ltp
 */

#ifndef BSP_DEBUG_H_
#define BSP_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

// printf wrapper that can be turned on and off by defining/undefining
#ifdef BSP_DEBUG_MODE

  #define BSP_printf(...)  do { printf("\r[BSP] " __VA_ARGS__); fflush(stdout); } while(0)

  #define BSP_assert(x)  \
  { \
    if( (x) == 0) { \
      fprintf(stderr, " ASSERT (%s|%s|%d)\r\n", __FILE__, __func__, __LINE__); \
      while (getchar()!='q')  \
        ; \
    } \
  }

    #define UTILS_assert(x)   BSP_assert(x)

#define BSP_DEBUG \
  fprintf(stderr, " %s:%s:%d Press Any key to continue !!!", __FILE__, __func__, __LINE__);


#define BSP_DEBUG_WAIT \
  BSP_DEBUG \
  getchar();

#define BSP_COMPILETIME_ASSERT(condition)                                       \
                   do {                                                         \
                       typedef char ErrorCheck[((condition) == TRUE) ? 1 : -1]; \
                   } while(0)

#else

  #define BSP_printf(...)
  #define BSP_assert(x)
  #define UTILS_assert(x)
  #define BSP_DEBUG
  #define BSP_DEBUG_WAIT
#endif

// printf wrapper that can be used to display errors. Prefixes all text with
// "ERROR" and inserts the file and line number of where in the code it was
// called
#define BSP_ERROR(...) \
  do \
  { \
  fprintf(stderr, " ERROR  (%s|%s|%d): ", __FILE__, __func__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
  } \
  while(0);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_DEBUG_H_ */
