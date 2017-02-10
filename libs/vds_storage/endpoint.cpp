/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "endpoint.h"

static vds::endpoint endpoints[] =
{
  vds::endpoint { "127.0.0.1", 8001 }
};

vds::storage_cursor<vds::endpoint>::storage_cursor(istorage &)
  : index_(0)
{
}

bool vds::storage_cursor<vds::endpoint>::read()
{
  this->index_++;
  return (sizeof(endpoints) / sizeof(endpoints[0]) >= this->index_);
}

vds::endpoint & vds::storage_cursor<vds::endpoint>::current()
{
  if (
    0 == this->index_
    || sizeof(endpoints) / sizeof(endpoints[0]) < this->index_
  ) {
    throw new std::out_of_range("storage_cursor<vds::endpoint>::current()");
  }

  return endpoints[this->index_ - 1];
}
