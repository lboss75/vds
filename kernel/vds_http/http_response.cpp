/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_response.h"

vds::http_response::http_response(
  int code,
  const std::string & comment)
: protocol_("HTTP/1.0"), code_(code), comment_(comment)
{
}

vds::http_response::http_response(
  int code,
  const std::string & comment,
  const std::string & body)
: protocol_("HTTP/1.0"), code_(code), comment_(comment), body_(body)
{
}

vds::http_response::http_response(
  int code,
  const std::string & comment,
  const filename & fn)
  : protocol_("HTTP/1.0"), code_(code), comment_(comment), file_(fn)
{
}

std::shared_ptr<vds::http_message> vds::http_response::create_message() const
{
  std::list<std::string> headers(this->headers_);

  std::stringstream stream;
  stream << this->protocol_ << " " << this->code_ << " " << this->comment_;
  headers.push_front(stream.str());

  if (!this->file_.empty()) {
    headers.push_back("Content-Length: " + std::to_string(file::length(this->file_)));
  }
  else {
    headers.push_back("Content-Length: " + std::to_string(this->body_.length()));
  }


  if (!this->file_.empty()) {
    return std::make_shared<http_message>(headers, this->file_);
  }
  else {
    return std::make_shared<http_message>(headers, this->body_);
  }
}



