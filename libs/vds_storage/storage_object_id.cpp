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
: source_server_id_(original.source_server_id_),
  index_(original.index_),
  signature_(original.signature_)
{
}

vds::storage_object_id::storage_object_id(
  const guid & source_server_id,
  uint64_t index,
  const data_buffer & signature)
: source_server_id_(source_server_id),
  index_(index),
  signature_(signature)
{
}

vds::storage_object_id::storage_object_id(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("s", this->source_server_id_);
    s->get_property("i", this->index_);
    s->get_property("g", this->signature_);
  }
}

vds::storage_object_id & vds::storage_object_id::operator=(storage_object_id && original)
{
  this->source_server_id_ = std::move(original.source_server_id_);
  this->index_ = original.index_;
  this->signature_ = std::move(original.signature_);

  return *this;
}

std::unique_ptr<vds::json_value> vds::storage_object_id::serialize(bool write_type) const
{
  std::unique_ptr<json_object> s(new json_object());
  if (write_type) {
    s->add_property("$t", message_type);
  }

  s->add_property("s", this->source_server_id_);
  s->add_property("i", this->index_);
  s->add_property("g", this->signature_);

  return std::unique_ptr<json_value>(s.release());
}
