
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "windows_event.h"
#include "windows_exception.h"

#ifdef _WIN32
vds::windows_event::system_resource::system_resource(BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName, LPSECURITY_ATTRIBUTES lpEventAttributes)
{
    this->handle_ = CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
    if (NULL == this->handle_) {
        auto error = GetLastError();
        throw new windows_exception("CreateEvent", error);
    }
}

vds::windows_event::system_resource::~system_resource()
{
    if (NULL != this->handle_) {
        CloseHandle(this->handle_);
    }
}

vds::windows_event::windows_event(BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName, LPSECURITY_ATTRIBUTES lpEventAttributes)
    : handle_(new system_resource(bManualReset, bInitialState, lpName, lpEventAttributes))
{
}

void vds::windows_event::set()
{
    if (!SetEvent(this->handle())) {
        auto error = GetLastError();
        throw new windows_exception("SetEvent", error);
    }
}

void vds::windows_event::wait() const
{
    WaitForSingleObject(this->handle(), INFINITE);
}
bool vds::windows_event::wait(DWORD timeout) const
{
    auto result = WaitForSingleObject(this->handle(), timeout);
    switch (result) {
    case WAIT_TIMEOUT:
        return false;
    case WAIT_OBJECT_0:
        return true;
    default:
        {
            auto error = GetLastError();
            throw new windows_exception("WaitForSingleObject", error);
        }
    }
}
#endif
