#ifndef __VDS_CORE_STRING_FORMAT_H_
#define __VDS_CORE_STRING_FORMAT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <sstream> 
#include <string>
#include <memory>

namespace vds {

template<typename ... Args>
inline std::string string_format( const std::string& format, Args ... args ) {
    size_t size = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    std::snprintf(buf.get(), size, format.c_str(), args ... );
    return std::string(buf.get(), buf.get() + size - 1);
}

}

#endif//__VDS_CORE_STRING_FORMAT_H_
