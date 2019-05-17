/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "http_message.h"

vds::http_message::http_message(const std::list<std::string>& headers)
	: headers_(headers)
{
	vds_assert(0 != headers.size());

	int index = 0;
	for (auto ch : *(headers.begin())) {
		if (isspace(ch)) {
			if (index < 2) {
				++index;
			}
			else {
				this->agent_ += ch;
			}
		}
		else {
			switch (index) {
			case 0:
				this->method_ += ch;
				break;

			case 1:
				this->url_ += ch;
				break;

			case 2:
				this->agent_ += ch;
				break;
			}
		}
	}

	this->parse_parameters();
}

vds::http_message::http_message(const std::string & method, const std::string & url, const std::list<std::string>& headers, const std::string & agent)
	: headers_(headers), method_(method), url_(url), agent_(agent)
{
	this->headers_.push_front(method + " " + url + " " + agent);
}

std::string vds::http_message::get_parameter(const std::string & name) const
{
	for (auto & param : this->parameters_) {
		auto p = param.find('=');
		if (std::string::npos != p && name == param.substr(0, p)) {
			return url_encode::decode(param.substr(p + 1));
		}
	}
	return std::string();
}

bool vds::http_message::get_header(
    const std::list<std::string> & headers,
    const std::string& name,
    std::string& value)
{
  for (auto& p : headers) {
    //Start with
    if (
        p.size() > name.size()
        && p[name.size()] == ':'
        && !p.compare(0, name.size(), name)) {
      value = p.substr(name.size() + 1);
      trim(value);
      return true;
    }
  }

  return false;
}

bool vds::http_message::have_header(const std::list<std::string>& headers, const std::string & name)
{
	for (auto& p : headers) {
		//Start with
		if (
			p.size() > name.size()
			&& p[name.size()] == ':'
			&& !p.compare(0, name.size(), name)) {
			return true;
		}
	}

	return false;
}

void vds::http_message::parse_parameters()
{
	auto p = strchr(this->url_.c_str(), '?');
	if (nullptr != p) {
		this->parameters_ = split_string(p + 1, '&', false);
		this->url_.erase(p - this->url_.c_str());
	}
}

