/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "node.h"
#include "storage_log.h"
#include "private/storage_log_p.h"

vds::node::node(
  const guid & id,
  const std::string & certificate)
  : id_(id),
  certificate_(certificate)
{
}

