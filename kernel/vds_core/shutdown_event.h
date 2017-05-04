#ifndef __VDS_CORE_SHUTDOWN_EVENT_H_
#define __VDS_CORE_SHUTDOWN_EVENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "windows_event.h"
#include "cancellation_token.h"

namespace vds {
  class shutdown_event
  {
  public:
    shutdown_event();
    ~shutdown_event();

    bool is_shuting_down() const { return this->token_.is_cancellation_requested(); }

    void set();
    void then_shuting_down(
      const std::function<void(void)> & callback)
    {
      this->token_.then_cancellation_requested(callback);
    }


  private:
    cancellation_token_source source_;
    cancellation_token token_;

#ifdef _WIN32
  public:
    HANDLE windows_handle() const {
      return event_.handle();
    }

  private:
    windows_event event_;
#endif // _WIN32
  };
}

#endif // !__VDS_CORE_SHUTDOWN_EVENT_H_


