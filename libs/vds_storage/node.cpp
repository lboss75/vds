/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "node.h"
#include "storage_log.h"
#include "storage_log_p.h"

vds::node::node(
  const std::string & id,
  const std::string & certificate)
  : id_(id),
  certificate_(certificate)
{
}



vds::storage_cursor<vds::node>::storage_cursor(const istorage & storage)
  : _simple_storage_cursor<node>(storage.get_storage_log()->get_nodes())
{
}
