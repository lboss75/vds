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

vds::async_task<std::shared_ptr<vds::http_message>> vds::http_router::route(
  const service_provider & sp,
  const std::shared_ptr<http_message> & message) const
{
  return create_async_task(
    [this, message](
    const std::function<void(const vds::service_provider & sp, std::shared_ptr<http_message> response)> & done,
    const error_handler & on_error,
    const service_provider & sp) {

    http_request request(message);

    auto p = this->static_.find(request.url());
    if (this->static_.end() != p) {
      done(sp, http_response::simple_text_response(sp, p->second));
      return;
    }

    auto pf = this->files_.find(request.url());
    if (this->files_.end() != pf) {
      throw std::runtime_error("Not implemented");
      http_response response(http_response::HTTP_OK, "OK");

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

      done(sp, response.create_message());
      return;
    }

    sp.get<logger>()->debug("HTTP", sp, "File not found: %s", request.url().c_str());
    done(sp, http_response(http_response::HTTP_Not_Found, "Not Found").create_message());
  });
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


