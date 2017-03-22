/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "cert.h"
#include "storage_log.h"
#include "storage_log_p.h"

vds::storage_cursor<vds::cert>::storage_cursor(const istorage & storage)
  : _simple_storage_cursor<cert>(storage.get_storage_log()->get_certificates())
{
}

vds::cert::cert(
  const std::string & object_name,
  const std::string & source_server_id,
  uint64_t object_id,
  const std::string & password_hash)
  : object_name_(object_name),
  source_server_id_(source_server_id),
  object_id_(object_id),
  password_hash_(password_hash)
{
}
