#include "stdafx.h"
#include "http_response.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

vds::http_response::http_response(
  const std::function<void(
    const simple_done_handler_t & done,
    const error_handler_t & on_error,
    const std::shared_ptr<http_response> & response)> & complete_handler,
  const std::shared_ptr<http_request> & request
): complete_handler_(complete_handler)
{
}
