/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_multipart_request.h"

vds::http_multipart_request::http_multipart_request(const std::string& method, const std::string& url,
  const std::string& agent) {
  this->headers_.push_back(method + " " + url + " " + agent);
}

void vds::http_multipart_request::add_string(const std::string& name, const std::string& value) {

}

void vds::http_multipart_request::add_file(const filename& body_file, const std::string& filename,
  const std::string& content_type) {
}

vds::http_message vds::http_multipart_request::get_message() {
  return http_message(this->headers_, std::make_shared<multipart_body>());
}

vds::async_task<size_t> vds::http_multipart_request::multipart_body::read_async(uint8_t* buffer, size_t len) {
  co_return 0;
}


