#ifndef __VDS_CORE_SHUTDOWN_EVENT_H_
#define __VDS_CORE_SHUTDOWN_EVENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "windows_event.h"
#include "events.h"

namespace vds {
    class shutdown_event : public event_source<>
    {
    public:
        shutdown_event();
        ~shutdown_event();

        bool is_shuting_down() const;

        void set();
#ifdef _WIN32
        HANDLE windows_handle() const {
            return event_.handle();
        }

    private:
        windows_event event_;
#else
    private:
        bool is_shuting_down_;
#endif // _WIN32


    };
}

#endif // !__VDS_CORE_SHUTDOWN_EVENT_H_


