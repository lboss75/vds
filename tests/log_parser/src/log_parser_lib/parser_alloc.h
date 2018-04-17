#ifndef PARSER_ALLOC_H
#define PARSER_ALLOC_H

#include <cstdlib>

class parser_alloc {
public:

  class no_throw_t {
  };

  static const no_throw_t no_throw;
};


inline void * operator new (size_t size, parser_alloc::no_throw_t) {
  return malloc(size);
}

inline void operator delete (void * ptr, parser_alloc::no_throw_t) {
  if (nullptr != ptr) {
    free(ptr);
  }
}


#endif//PARSER_ALLOC_H