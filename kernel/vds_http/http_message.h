#ifndef __VDS_HTTP_HTTP_MESSAGE_H_
#define __VDS_HTTP_HTTP_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>

#include "async_buffer.h"

namespace vds {
  class http_message
  {
  public:
    http_message() {
    }

	http_message(
		const std::list<std::string> & headers);

	http_message(
		const std::string & method,
		const std::string & url,
		const std::list<std::string> & headers = std::list<std::string>(),
		const std::string & agent = "HTTP/1.0");

	const std::string & url() const {
		return this->url_;
	}

	const std::string & method() const {
		return this->method_;
	}
	const std::string & agent() const {
		return this->agent_;
	}

	std::string get_parameter(const std::string & name) const;

    const std::list<std::string> & headers() const {
      return this->headers_;
    }

    static bool get_header(
        const std::list<std::string> & headers,
        const std::string& name,
        std::string& value);

	static bool have_header(
		const std::list<std::string> & headers,
		const std::string& name);

    bool get_header(const std::string & name, std::string & value) const{
      return get_header(this->headers_, name, value);
    }
    
    operator bool () const {
      return !this->headers_.empty();
    }

    bool operator ! () const {
      return this->headers_.empty();
    }

  private:
	  std::list<std::string> headers_;

	  std::string method_;
	  std::string url_;
	  std::list<std::string> parameters_;
	  std::string agent_;


	  void parse_parameters();
  };
}
#endif // __VDS_HTTP_HTTP_MESSAGE_H_
