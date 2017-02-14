/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_router.h"
#include "http_response.h"
#include "http_outgoing_stream.h"
#include "http_request.h"

vds::http_router::http_router(const service_provider & sp)
  : log_(sp, "HTTP Router")
{
}

void vds::http_router::route(
  const http_request & request,
  http_incoming_stream & /*incoming_stream*/,
  http_response & response,
  http_outgoing_stream & outgoing_stream
) const
{
  auto p = this->static_.find(request.url());
  if(this->static_.end() != p){
    response.set_result(200, "OK");
    response.add_header("Content-Type", "text/html; charset=utf-8");
    outgoing_stream.set_body(p->second);
    return;
  }

  auto pf = this->files_.find(request.url());
  if (this->files_.end() != pf) {
    response.set_result(200, "OK");
    auto ext = pf->second.extension();
    if (".js" == ext) {
      response.add_header("Content-Type", "application/javascript; charset=utf-8");
    }
    else if (".css" == ext) {
      response.add_header("Content-Type", "text/css");
    }
    else if (".woff" == ext) {
      response.add_header("Content-Type", "application/font-woff");
    }
    else if (".ttf" == ext) {
      response.add_header("Content-Type", "application/font-tff");
    }
    else {
      response.add_header("Content-Type", "text/html; charset=utf-8");
    }
    outgoing_stream.set_file(pf->second);
    return;
  }
  
  this->log_(ll_debug, "File not found: %s", request.url().c_str());
  response.set_result(404, "Not Found");
  outgoing_stream.set_body("<html><body>File not file</body></html>");
}

void vds::http_router::add_static(
  const std::string& url,
  const std::string& response)
{
  this->static_[url] = response;
}

void vds::http_router::add_file(
  const std::string & url,
  const filename & filename
)
{
  this->files_[url] = filename;
}


