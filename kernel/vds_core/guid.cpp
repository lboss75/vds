/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "guid.h"
#include "encoding.h"

#include <stdexcept>
#include <system_error>

#ifndef WIN32
#include <uuid/uuid.h>
#endif

vds::guid::guid()
{
}

vds::guid::guid(const vds::guid& other)
: const_data_buffer(other)
{
}

vds::guid::guid(vds::guid&& other)
: const_data_buffer(std::move(other))
{
}

vds::guid::guid(const void* data, size_t len)
: const_data_buffer(data, len)
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
  if(nullptr == this->data() || 0 == this->size()){
    return "null";
  }
  
#ifdef WIN32
	auto guid = (const GUID *)this->data();
  char s[37];
  sprintf(s, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
	  guid->Data1, guid->Data2, guid->Data3,
	  guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
	  guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
  return s;
#else
  char s[37];
  uuid_unparse(*(const uuid_t *)this->data(), s);
  return s;
#endif
}

vds::guid& vds::guid::operator = (vds::guid&& original)
{
  const_data_buffer::operator=(original);
  return *this;
}

vds::guid& vds::guid::operator=(const vds::guid& original)
{
  const_data_buffer::operator=(original);
  return *this;
}

vds::guid vds::guid::parse(const std::string & value)
{
#ifdef WIN32
	GUID result;

	unsigned long p0;
	int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

	int err = sscanf_s(
		value.c_str(),
		"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		&p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
	if(11 != err)
	{
		auto e = errno;
		throw std::system_error(e, std::system_category());
	}
	result.Data1 = p0;
	result.Data2 = p1;
	result.Data3 = p2;
	result.Data4[0] = p3;
	result.Data4[1] = p4;
	result.Data4[2] = p5;
	result.Data4[3] = p6;
	result.Data4[4] = p7;
	result.Data4[5] = p8;
	result.Data4[6] = p9;
	result.Data4[7] = p10;
	return guid(&result, sizeof(result));

#else
  uuid_t result;
  uuid_parse(value.c_str(), result);
  return guid(&result, sizeof(result));
#endif
}

bool vds::guid::operator == (const guid & other) const
{
  return this->size() == other.size()
    && 0 == memcmp(this->data(), other.data(), this->size());
}

bool vds::guid::operator != (const guid & other) const
{
  return this->size() != other.size()
    || 0 != memcmp(this->data(), other.data(), this->size());
}

bool vds::guid::operator < (const guid & other) const
{
  return this->size() == other.size()
    && 0 > memcmp(this->data(), other.data(), this->size());
}

bool vds::guid::operator > (const guid & other) const
{
  return this->size() == other.size()
    && 0 < memcmp(this->data(), other.data(), this->size());
}
