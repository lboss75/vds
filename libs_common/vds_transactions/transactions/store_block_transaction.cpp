/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "store_block_transaction.h"

vds::expected<vds::transactions::store_block_transaction> vds::transactions::store_block_transaction::create(
  const const_data_buffer& owner_id,
  const const_data_buffer& object_id,
  uint64_t object_size,
  const std::vector<const_data_buffer>& replicas,
  const asymmetric_private_key & private_key)
{
  binary_serializer s;
  CHECK_EXPECTED(s << owner_id);
  CHECK_EXPECTED(s << object_id);
  CHECK_EXPECTED(s << object_size);
  CHECK_EXPECTED(s << replicas);

  GET_EXPECTED(owner_sig, asymmetric_sign::signature(hash::sha256(), private_key, s.move_data()));

  return message_create<transactions::store_block_transaction>(
    owner_id,
    object_id,
    object_size,
    replicas,
    owner_sig);
}
