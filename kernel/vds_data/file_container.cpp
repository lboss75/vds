/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "file_container.h"


vds::file_container& vds::file_container::add(const std::string & name, const std::string & body)
{
  this->items_.push_back(item{ name, body });

  return *this;
}

vds::binary_serializer & vds::file_container::serialize(binary_serializer & s) const
{
  s.write_number(this->items_.size());
  for (auto & p : this->items_) {
    s << p.name << p.body;
  }

  return s;
}
