#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include <map>
#include <stdafx.h>
#include "binary_serialize.h"
#include "guid.h"
#include "database.h"
#include "asymmetriccrypto.h"

namespace vds {
  namespace transactions {
    class transaction_block {
    public:

      template<typename item_type>
      void add(
          item_type && item) {

        this->s_ << item_type::message_id;
        item.serialize(this->s_);
      }

	  const_data_buffer save(
          const service_provider &sp,
          class vds::database_transaction & t,
          const certificate & common_read_cert,
          const certificate & write_cert,
          const asymmetric_private_key & write_private_key,
		  bool apply = true) const;

    private:

      binary_serializer s_;

      void collect_dependencies(
          class database_transaction &t,
          std::set<const_data_buffer> &ancestors) const;

      const_data_buffer register_transaction(
		  const service_provider & sp,
          class database_transaction &t,
          const const_data_buffer &block,
          const std::set<const_data_buffer> &ancestors) const;

      void on_new_transaction(
          const service_provider &sp,
          class database_transaction &t,
          const const_data_buffer & id,
          const const_data_buffer &block) const;
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
