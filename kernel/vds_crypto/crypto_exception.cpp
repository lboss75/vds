/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "crypto_exception.h"
#include "string_format.h"

vds::crypto_exception::crypto_exception(const std::string & message, int error_code)
  : std::runtime_error(format_message(message, error_code))
{
  std::string error = this->what();
  std::cout << error << "\n";
}

std::string vds::crypto_exception::format_message(const std::string& message, int error_code)
{
  auto result = string_format("%s: error %d", message.c_str(), error_code);
  
  while(SSL_ERROR_NONE != error_code){
    char errorString[1024];
    ERR_error_string_n(error_code, errorString, 1024);
    
    result += ", ";
    result += errorString;
    
    error_code = ERR_get_error();
  }
  
  return result;
}

