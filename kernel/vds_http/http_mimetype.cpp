/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_mimetype.h"

static const char * mime_types[] = {
  ".js", "application/javascript",
  ".css", "text/css",
  ".woff", "application/font-woff",
  ".ttf", "application/font-tff",
  ".html", "text/html",
  nullptr
};



std::string vds::http_mimetype::mimetype(const filename& fn) {
  const auto ext = fn.extension();

  auto p = mime_types;
  while(*p != nullptr) {
    if(str_equal_ignore_case(*p++, ext)) {
      return *p;
    }
    ++p;
  }

  return std::string();
}
