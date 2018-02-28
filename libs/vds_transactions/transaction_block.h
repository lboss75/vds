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

        auto & s = this->data_[channel_id];
        s << item_type::message_id;
        item.serialize(s);
      }

      std::map<guid, const_data_buffer> save(
          const service_provider &sp,
          class vds::database_transaction & t,
          const std::function<void(
              const guid & channel_id,
              certificate & read_cert,
              certificate & write_cert,
              asymmetric_private_key & write_private_key)> & crypto_callback) const;

      static void parse_block(const const_data_buffer &data, guid &channel_id, uint64_t &order_no, guid &read_cert_id,
                                   guid &write_cert_id, std::set<const_data_buffer> &ancestors, const_data_buffer &crypted_data,
                                   const_data_buffer &crypted_key, const_data_buffer &signature);

      static bool validate_block(const certificate &write_cert, const guid &channel_id, uint64_t &order_no, const guid &read_cert_id,
                                       const guid &write_cert_id, const std::set<const_data_buffer> &ancestors,
                                       const const_data_buffer &crypted_data, const const_data_buffer &crypted_key,
                                       const const_data_buffer &signature);

    private:
      std::map<guid, binary_serializer> data_;

      uint64_t collect_dependencies(
          class database_transaction &t,
          const guid & channel_id,
          std::set<const_data_buffer> &ancestors) const;

      const_data_buffer register_transaction(
		  const service_provider & sp,
          class database_transaction &t,
          const guid & channel_id,
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
