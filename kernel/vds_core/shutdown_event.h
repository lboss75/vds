#ifndef __VDS_CORE_SHUTDOWN_EVENT_H_
#define __VDS_CORE_SHUTDOWN_EVENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "windows_event.h"

namespace vds {
  class shutdown_event
  {
  public:
    shutdown_event();
    ~shutdown_event();

    expected<void> create();

    bool is_shuting_down() const { return this->is_shuting_down_; }

    vds::expected<void> set();

  private:
    bool is_shuting_down_;

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


