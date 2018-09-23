#ifndef __VDS_HTTP_HTTP_MIDDLEWARE_H_
#define __VDS_HTTP_HTTP_MIDDLEWARE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"
#include "http_router.h"
#include "http_request.h"
#include "http_response.h"

namespace vds {
  class http_router;

  template <typename router_type>
  class http_middleware
  {
  public:
    http_middleware(router_type & router)
    : router_(router)
    {
    }

    ~http_middleware()
    {
    }
    
    vds::async_task<http_message> process(
      const vds::service_provider & sp,
      const http_message & request)
    {
      return this->router_.route(sp, request);
    }

  private:
    router_type & router_;
  };
}

#endif // __VDS_HTTP_HTTP_MIDDLEWARE_H_
