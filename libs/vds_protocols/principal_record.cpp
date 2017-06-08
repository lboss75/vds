/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "principal_record.h"
#include "storage_log.h"
#include "storage_log_p.h"

vds::principal_record::principal_record(
  const guid & parent_principal,
  const guid & id,
  const std::string & cert_body,
  const std::string & cert_key,
  const const_data_buffer & password_hash)
: parent_principal_(parent_principal),
  id_(id),
  cert_body_(cert_body),
  cert_key_(cert_key),
  password_hash_(password_hash)
{
}
