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

std::string vds::guid::str() const
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

vds::guid& vds::guid::operator = (vds::guid&& original)
{
  data_buffer::operator=(std::move(original));
  return *this;
}

vds::guid& vds::guid::operator=(const vds::guid& original)
{
  data_buffer::operator=(original);
  return *this;
}

vds::guid vds::guid::parse(const std::string & value)
{
#ifdef WIN32
  GUID result;
  HRESULT hr = IIDFromString(utf16::from_utf8(value).c_str(), &result);
  if (S_OK != hr) {
    throw new std::system_error(hr, std::system_category(), "Invalid GUID " + value);
  }

  return guid(&result, sizeof(result));
#else
  uuid_t result;
  uuid_parse(value.c_str(), result);
  return guid(&result, sizeof(result));
#endif
}
