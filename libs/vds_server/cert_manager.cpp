/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "cert_manager.h"
#include "private/cert_manager_p.h"

bool vds::cert_manager::validate(
  const certificate& /*cert*/)
{
  return true;
}

////////////////////////////////////
bool vds::_cert_manager::validate(
  const certificate& /*cert*/)
{
  return true;
}

void vds::_cert_manager::create_database_objects(
  const service_provider & /*sp*/,
  uint64_t db_version,
  database_transaction & t)
{
  if (1 > db_version) {
    t.execute(
      "CREATE TABLE cert(\
      id VARCHAR(64) NOT NULL,\
      body BLOB NOT NULL,\
      parent VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_cert PRIMARY KEY(id))");
  }
}
