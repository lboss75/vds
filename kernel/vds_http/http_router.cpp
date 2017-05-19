/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_router.h"
#include "http_response.h"
#include "http_outgoing_stream.h"
#include "http_request.h"

vds::http_router::http_router()
{
}

std::shared_ptr<vds::http_message> vds::http_router::route(
  const service_provider & sp,
  const std::shared_ptr<http_message> & message)
{
  http_request request(message);

  auto p = this->static_.find(request.url());
  if(this->static_.end() != p){
    http_response response(http_response::HTTP_OK, "OK", p->second);
    response.add_header("Content-Type", "text/html; charset=utf-8");
    return response.create_message();
  }

  auto pf = this->files_.find(request.url());
  if (this->files_.end() != pf) {
    http_response response(http_response::HTTP_OK, "OK", pf->second);

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

    return response.create_message();
  }
  
  sp.get<logger>()->debug(sp, "File not found: %s", request.url().c_str());
  return http_response(http_response::HTTP_Not_Found, "Not Found", "<html><body>File not file</body></html>").create_message();;
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


