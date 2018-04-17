#ifndef __LOG_PARSER_LIB_PARSER_ALLOC_H_
#define __LOG_PARSER_LIB_PARSER_ALLOC_H_

#include <cstdlib>

class parser_alloc {
public:

  class no_throw_t {
  };

  static const no_throw_t no_throw;
};

//Redefine operator new to avoid exceptions
inline void * operator new (size_t size, parser_alloc::no_throw_t) {
  return malloc(size);
}

inline void operator delete (void * ptr, parser_alloc::no_throw_t) {
  if (nullptr != ptr) {
    free(ptr);
  }
}


#endif//__LOG_PARSER_LIB_PARSER_ALLOC_H_