/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "principal_record.h"

vds::principal_record::principal_record(
  const guid & parent_principal,
  const guid & id,
  const certificate & cert_body,
  const const_data_buffer & cert_key,
  const const_data_buffer & password_hash)
: parent_principal_(parent_principal),
  id_(id),
  cert_body_(cert_body),
  cert_key_(cert_key),
  password_hash_(password_hash)
{
}
