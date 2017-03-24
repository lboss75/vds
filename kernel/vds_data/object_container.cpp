/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "object_container.h"

vds::object_container::object_container()
{
}

vds::object_container::object_container(binary_deserializer & s)
{
  auto count = s.read_number();
  for (uint64_t i = 0; i < count; ++i) {
    std::string name;
    std::string body;

    s >> name >> body;

    this->items_[name] = body;
  }
}

vds::object_container::object_container(binary_deserializer && s)
{
  auto count = s.read_number();
  for (uint64_t i = 0; i < count; ++i) {
    std::string name;
    std::string body;

    s >> name >> body;

    this->items_[name] = body;
  }
}

vds::object_container& vds::object_container::add(const std::string & name, const std::string & body)
{
  this->items_[name] = body;
  return *this;
}

vds::binary_serializer & vds::object_container::serialize(binary_serializer & s) const
{
  s.write_number(this->items_.size());
  for (auto & p : this->items_) {
    s << p.first << p.second;
  }

  return s;
}

std::string vds::object_container::get(const std::string & name) const
{
  auto p = this->items_.find(name);
  if (this->items_.end() == p) {
    return std::string();
  }
  else {
    return p->second;
  }
}