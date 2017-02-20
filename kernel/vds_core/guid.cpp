/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "guid.h"
#include "encoding.h"

std::string vds::guid::new_guid_string()
{
#ifdef WIN32
  GUID data;
  CoCreateGuid(&data);

  OLECHAR * bstrGuid;
  StringFromCLSID(data, &bstrGuid);

  std::string result = utf16::to_utf8(bstrGuid);

  ::CoTaskMemFree(bstrGuid);

  return result;
#else
  uuid_t uuid;
  uuid_generate_random(uuid);
  char s[37];
  uuid_unparse(uuid, s);
  return s;
#endif
}
