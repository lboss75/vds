/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
//#include "http_request.h"
//
//vds::http_request::http_request(const http_message & message)
//  : message_(message)
//{
//  vds_assert(0 != message.headers().size());
//
//  int index = 0;
//  for (auto ch : *(message.headers().begin())) {
//    if (isspace(ch)) {
//      if (index < 2) {
//        ++index;
//      }
//      else {
//        this->agent_ += ch;
//      }
//    }
//    else {
//      switch (index) {
//      case 0:
//        this->method_ += ch;
//        break;
//
//      case 1:
//        this->url_ += ch;
//        break;
//
//      case 2:
//        this->agent_ += ch;
//        break;
//      }
//    }
//  }
//
//  this->parse_parameters();
//}
//
//void vds::http_request::parse_parameters()
//{
//  this->parameters_.clear();
//
//  auto p = strchr(this->url_.c_str(), '?');
//  if (nullptr != p) {
//    this->parameters_ = split_string(p + 1, '&', false);
//    this->url_.erase(p - this->url_.c_str());
//  }
//}
//
//vds::http_message vds::http_request::simple_request(
//  
//  const std::string& method,
//  const std::string& url,
//  const std::string& body)
//{
//  std::list<std::string> headers;
//  headers.push_back(method + " " + url + " HTTP/1.0");
//  headers.push_back("Content-Type: text/html; charset=utf-8");
//  headers.push_back("Content-Length:" + std::to_string(body.length()));
//
//  return http_message(headers, std::make_shared<buffer_stream_input_async>(const_data_buffer(body.c_str(), body.length())));
//}
//
//std::string vds::http_request::get_parameter(const std::string &name) const {
//  for(auto & param : this->parameters_){
//    auto p = param.find('=');
//    if(std::string::npos != p && name == param.substr(0, p)){
//      return url_encode::decode(param.substr(p + 1));
//    }
//  }
//  return std::string();
//}
