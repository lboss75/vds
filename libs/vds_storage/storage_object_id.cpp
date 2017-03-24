/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_object_id.h"

const char vds::storage_object_id::message_type[] = "object id";

vds::storage_object_id::storage_object_id()
{
}

vds::storage_object_id::storage_object_id(const storage_object_id & original)
: index_(original.index_),
  signature_(original.signature_)
{
}

vds::storage_object_id::storage_object_id(
  uint64_t index,
  const data_buffer & signature)
: index_(index),
  signature_(signature)
{
}


vds::storage_object_id::storage_object_id(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("i", this->index_);
    s->get_property("g", this->signature_);
  }
}

vds::storage_object_id & vds::storage_object_id::operator=(storage_object_id && original)
{
  this->index_ = original.index_;
  this->signature_ = std::move(original.signature_);

  return *this;
}

std::unique_ptr<vds::json_value> vds::storage_object_id::serialize(bool write_type) const
{
  std::unique_ptr<json_object> s(new json_object());
  
  this->serialize(s.get(), write_type);

  return std::unique_ptr<json_value>(s.release());
}

void vds::storage_object_id::serialize(vds::json_object* s, bool write_type) const
{
  if (write_type) {
    s->add_property("$t", message_type);
  }

  s->add_property("i", this->index_);
  s->add_property("g", this->signature_);
}

/////////////////////////////////////////////////////////////////////////////
const char vds::full_storage_object_id::message_type[] = "full object id";

vds::full_storage_object_id::full_storage_object_id()
{
}

vds::full_storage_object_id::full_storage_object_id(
  const full_storage_object_id & original)
: storage_object_id(original),
  source_server_id_(original.source_server_id_)
{

}


vds::full_storage_object_id::full_storage_object_id(const json_value* source)
: storage_object_id(source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("s", this->source_server_id_);
  }
}

vds::full_storage_object_id::full_storage_object_id(
  const guid& source_server_id,
  uint64_t index,
  const data_buffer& signature)
: storage_object_id(index, signature),
  source_server_id_(source_server_id)
{
}

vds::full_storage_object_id::full_storage_object_id(
  const vds::guid& source_server_id,
  const vds::storage_object_id& object_id)
: storage_object_id(object_id),
  source_server_id_(source_server_id)
{

}

std::unique_ptr<vds::json_value> vds::full_storage_object_id::serialize(bool write_type) const
{
  std::unique_ptr<json_object> s(new json_object());
  
  if (write_type) {
    s->add_property("$t", message_type);
  }

  storage_object_id::serialize(s.get(), false);
  s->add_property("s", this->source_server_id_);

  return std::unique_ptr<json_value>(s.release());
}

vds::full_storage_object_id& vds::full_storage_object_id::operator = (vds::full_storage_object_id&& original)
{
  this->source_server_id_ = std::move(original.source_server_id_);
  storage_object_id::operator=(std::move(original));
  
  return *this;
}
