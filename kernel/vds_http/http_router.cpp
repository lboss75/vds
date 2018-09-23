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

vds::async_task<vds::http_message> vds::http_router::route(
  const service_provider & sp,
  const http_message & message,
  const std::string & local_path) const
{
    http_request request(message);

    auto p = this->static_.find(local_path);
    if (this->static_.end() != p) {
      co_await message.ignore_empty_body(sp);
      co_return http_response::simple_text_response(p->second);
    }

    auto pf = this->files_.find(local_path);
    if (this->files_.end() != pf) {
      auto ext = pf->second.extension();
      std::string content_type;
      if (".js" == ext) {
        content_type = "application/javascript; charset=utf-8";
      }
      else if (".css" == ext) {
        content_type = "text/css";
      }
      else if (".woff" == ext) {
        content_type = "application/font-woff";
      }
      else if (".ttf" == ext) {
        content_type = "application/font-tff";
      }
      else {
        content_type = "text/html; charset=utf-8";
      }

      co_await message.ignore_empty_body(sp);
      co_return http_response::simple_text_response(file::read_all_text(pf->second), content_type);
    }

    sp.get<logger>()->debug("HTTP", sp, "File not found: %s", local_path.c_str());
    co_await message.ignore_empty_body(sp);
    co_return http_response::simple_text_response(
      std::string(),
      std::string(),
      http_response::HTTP_Not_Found,
      "Not Found");
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


