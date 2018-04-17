#ifndef __LOG_PARSER_LIB_PARSER_DEBUG_H_
#define __LOG_PARSER_LIB_PARSER_DEBUG_H_
#include <cstdio>

template<typename ... Args>
inline void parser_debug(const char * format, Args ... args) {
#if defined(_DEBUG) || defined(DEBUG)
  printf(format, args...);
#endif
}

#endif//__LOG_PARSER_LIB_PARSER_DEBUG_H_