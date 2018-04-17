#pragma once
#include <cstdio>

template<typename ... Args>
inline void parser_debug(const char * format, Args ... args) {
#ifdef _DEBUG
  printf(format, args...);
#endif
}
