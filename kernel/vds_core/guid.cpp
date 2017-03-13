/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "guid.h"
#include "encoding.h"

vds::guid::guid()
{
}

vds::guid::guid(const vds::guid& other)
: data_buffer(other)
{
}

vds::guid::guid(vds::guid&& other)
: data_buffer(std::move(other))
{
}

vds::guid::guid(const void* data, size_t len)
: data_buffer(data, len)
{
}


vds::guid vds::guid::new_guid()
{
#ifdef WIN32
  GUID data;
  CoCreateGuid(&data);
  return guid(&data, sizeof(data));
#else
  uuid_t uuid;
  uuid_generate_random(uuid);
  return guid(&uuid, sizeof(uuid));
#endif
}

std::string vds::guid::str()
{
  if(nullptr == this->data()){
    return "null";
  }
  
#ifdef WIN32
  OLECHAR * bstrGuid;
  StringFromCLSID(*(const GUID *)this->data(), &bstrGuid);

  std::string result = utf16::to_utf8(bstrGuid);

  ::CoTaskMemFree(bstrGuid);

  return result;
#else
  char s[37];
  uuid_unparse(*(const uuid_t *)this->data(), s);
  return s;
#endif
}
