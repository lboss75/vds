#ifndef __VDS_CORE_WINDOWS_EVENT_H_
#define __VDS_CORE_WINDOWS_EVENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifdef _WIN32
#include <memory>
#include <string>

namespace vds {

    class windows_event
    {
    public:
        windows_event(
            BOOL bManualReset = FALSE,
            BOOL bInitialState = FALSE,
            LPCTSTR lpName = NULL,
            LPSECURITY_ATTRIBUTES lpEventAttributes = NULL
            );

        HANDLE handle() const {
            return this->handle_->handle();
        }

        void set();

        void wait() const;

        bool wait(DWORD timeout) const;
    private:

        class system_resource : public std::enable_shared_from_this<system_resource>
        {
        public:
            system_resource(
                BOOL bManualReset = FALSE,
                BOOL bInitialState = FALSE,
                LPCTSTR lpName = NULL,
                LPSECURITY_ATTRIBUTES lpEventAttributes = NULL
                );

            ~system_resource();

            HANDLE handle() const {
                return this->handle_;
            }

        private:
            HANDLE handle_;
        };

        std::shared_ptr<system_resource> handle_;
    };
}
#endif //_WIN32
#endif // !__VDS_CORE_WINDOWS_EVENT_H_
