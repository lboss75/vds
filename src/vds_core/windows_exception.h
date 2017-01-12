#ifndef __VDS_CORE_WINDOWS_EXCEPTION_H_
#define __VDS_CORE_WINDOWS_EXCEPTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifdef _WIN32        
#include <exception>
#include <string>

namespace vds {
    class windows_exception : public std::exception {
        public:
            windows_exception(const std::string & message, DWORD errorCode);
        private:
            static std::string error_message(DWORD errorCode);

    };
}

#endif
#endif//__VDS_CORE_WINDOWS_EXCEPTION_H_
