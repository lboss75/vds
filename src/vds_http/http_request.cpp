#include "stdafx.h"
#include "http_request.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

vds::http_request::http_request(
  const std::string & method,
  const std::string & url,
  const std::string & agent,
  const std::list<std::string> & headers)
  : method_(method), url_(url), agent_(agent),
  headers_(headers)
{
}

void vds::http_request::push_data(
  const std::function<void(size_t readed, bool continue_read)> & done,
  const vds::error_handler_t & on_error,
  const char * data,
  size_t len
)
{
  auto readed = this->last_read_done_(data, len);
  done(readed, readed == len);
}

