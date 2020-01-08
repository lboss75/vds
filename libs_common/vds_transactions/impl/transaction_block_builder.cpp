/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "transaction_block_builder.h"

vds::const_data_buffer vds::transactions::transaction_block_builder::close()
{
  return this->data_.move_data();
}
