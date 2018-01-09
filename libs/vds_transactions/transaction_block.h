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
          const guid & channel_id,
          item_type && item) {

        auto & s = this->channels_[channel_id];

        s << item_type::message_id;
        item.serialize(s);
      }

      void save(
          const service_provider &sp,
          class vds::database_transaction & t,
          const certificate & common_read_cert,
          const certificate & write_cert,
          const asymmetric_private_key & write_private_key) const;

    private:

      std::map<guid, binary_serializer> channels_;

      void collect_dependencies(
          binary_serializer &s,
          class database_transaction &t,
          std::set<std::string> &ancestors) const;

      const_data_buffer register_transaction(
          class database_transaction &t,
          const std::set<std::string> &ancestors,
          const const_data_buffer &block) const;

      void on_new_transaction(
          const service_provider &sp,
          class database_transaction &t,
          const const_data_buffer & id,
          const const_data_buffer &block) const;
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
