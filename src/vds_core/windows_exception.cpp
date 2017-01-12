/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "windows_exception.h"
#include "string_format.h"

#ifdef _WIN32
vds::windows_exception::windows_exception(
    const std::string & message,
    DWORD errorCode)
    : std::exception(
        string_format("%s failed with error %d: %s",
            message.c_str(),
            errorCode,
            error_message(errorCode).c_str()).c_str()) {
    
}

std::string vds::windows_exception::error_message(DWORD errorCode) {
    LPVOID lpMsgBuf;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR) &lpMsgBuf,
        0, NULL );

    std::string result((LPSTR)lpMsgBuf);

    LocalFree(lpMsgBuf);

    return result;
}
#endif
