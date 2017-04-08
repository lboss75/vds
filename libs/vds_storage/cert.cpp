/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "cert.h"
#include "storage_log.h"
#include "storage_log_p.h"

vds::cert::cert(
  const std::string & object_name,
  const full_storage_object_id & object_id,
  const const_data_buffer & password_hash)
  : object_name_(object_name),
  object_id_(object_id),
  password_hash_(password_hash)
{
}
