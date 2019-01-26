#ifndef __VDS_CORE_WINDOWS_EVENT_H_
#define __VDS_CORE_WINDOWS_EVENT_H_
#include "expected.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifdef _WIN32
#include "targetver.h"
#include <memory>
#include <string>

namespace vds {

    class windows_event
    {
    public:
        windows_event();
        ~windows_event();

        expected<void> create(BOOL bManualReset = FALSE,
          BOOL bInitialState = FALSE,
          LPCTSTR lpName = NULL,
          LPSECURITY_ATTRIBUTES lpEventAttributes = NULL
        );

        HANDLE handle() const {
            return this->handle_;
        }

        expected<void> set();

        void wait() const;

        expected<bool> wait(DWORD timeout) const;
    private:
        HANDLE handle_;
    };
}
#endif //_WIN32
#endif // !__VDS_CORE_WINDOWS_EVENT_H_
