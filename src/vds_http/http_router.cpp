/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_router.h"
#include "http_response.h"

void vds::http_router::route(
  const simple_done_handler_t & done,
  const error_handler_t & on_error,
  std::shared_ptr<http_request> request,
  std::shared_ptr<http_response> response
)
{
  auto p = this->static_.find(request->url());
  if(this->static_.end() != p){
    response->set_result(200, "OK");
    response->add_header("Content-Type", "text/html; charset=utf-8");
    response->write_body(p->second);
    response->complete(
      done,
      on_error
    );
  }
  else {
    response->set_result(404, "Not Found");
    response->complete(
      done,
      on_error
    );
  }
}

void vds::http_router::add_static(
  const std::string& url,
  const std::string& response)
{
  this->static_[url] = response;
}
