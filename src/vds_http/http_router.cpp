/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_router.h"
#include "http_response.h"
#include "http_outgoing_stream.h"
#include "http_request.h"

void vds::http_router::route(
  const http_request & request,
  http_incoming_stream & incoming_stream,
  http_response & response,
  http_outgoing_stream & outgoing_stream
)
{
  auto p = this->static_.find(request.url());
  if(this->static_.end() != p){
    response.set_result(200, "OK");
    response.add_header("Content-Type", "text/html; charset=utf-8");
    outgoing_stream.set_body(p->second);
  }
  else {
    response.set_result(404, "Not Found");
  }
}

void vds::http_router::add_static(
  const std::string& url,
  const std::string& response)
{
  this->static_[url] = response;
}
