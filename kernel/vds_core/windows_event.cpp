
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "windows_event.h"

#ifdef _WIN32
vds::windows_event::windows_event()
: handle_(NULL) {
  
}

vds::windows_event::~windows_event()
{
  if (NULL != this->handle_) {
    CloseHandle(this->handle_);
  }
}

vds::expected<void> vds::windows_event::create(BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName, LPSECURITY_ATTRIBUTES lpEventAttributes)
{
  if (NULL != this->handle_) {
    CloseHandle(this->handle_);
  }

  this->handle_ = CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
  if (NULL == this->handle_) {
    auto error = GetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "CreateEvent");
  }

  return expected<void>();
}

vds::expected<void> vds::windows_event::set()
{
  if (!SetEvent(this->handle())) {
    auto error = GetLastError();
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "SetEvent");
  }

  return expected<void>();
}

void vds::windows_event::wait() const
{
    WaitForSingleObject(this->handle(), INFINITE);
}

vds::expected<bool> vds::windows_event::wait(DWORD timeout) const
{
    auto result = WaitForSingleObject(this->handle(), timeout);
    switch (result) {
    case WAIT_TIMEOUT:
        return false;
    case WAIT_OBJECT_0:
        return true;
    default: {
            auto error = GetLastError();
            return vds::make_unexpected<std::system_error>(error, std::system_category(), "WaitForSingleObject");
        }
    }
}
#endif
