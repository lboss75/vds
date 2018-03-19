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
#include "symmetriccrypto.h"

namespace vds {
  namespace transactions {
    class transaction_block {
    public:
      enum class block_type_t {
        normal = 0,
        self_signed = 1
      };

      template<typename item_type>
      void add(item_type && item) {

        this->data_ << item_type::message_id;
        item.serialize(this->data_);
      }

      void add_certificate(const certificate & cert) {
        this->certificates_.push_back(cert);
      }

      const_data_buffer save(
          const service_provider &sp,
          class vds::database_transaction &t,
          const const_data_buffer &channel_id,
          const certificate &read_cert,
          const certificate &write_cert,
          const asymmetric_private_key &write_private_key) const;

      const_data_buffer save_self_signed(
        const service_provider &sp,
        class vds::database_transaction &t,
        const const_data_buffer &channel_id,
        const certificate &write_cert,
        const asymmetric_private_key &write_private_key,
        const symmetric_key & user_password_key,
        const const_data_buffer & user_password_hash) const;

      static block_type_t parse_block(const const_data_buffer &data, const_data_buffer &channel_id, uint64_t &order_no, guid &read_cert_id,
                                   guid &write_cert_id, std::set<const_data_buffer> &ancestors, const_data_buffer &crypted_data,
                                   const_data_buffer &crypted_key, std::list<certificate> & certificates, const_data_buffer &signature);

      static bool validate_block(const certificate &write_cert, block_type_t block_type, const const_data_buffer &channel_id, uint64_t &order_no, const guid &read_cert_id,
                                       const guid &write_cert_id, const std::set<const_data_buffer> &ancestors,
                                       const const_data_buffer &crypted_data, const const_data_buffer &crypted_key,
                                       const std::list<certificate> & certificates,
                                       const const_data_buffer &signature);

    private:
      binary_serializer data_;
      std::list<certificate> certificates_;

      uint64_t collect_dependencies(
          class database_transaction &t,
          const const_data_buffer &channel_id,
          std::set<const_data_buffer> &ancestors) const;

      const_data_buffer register_transaction(
		      const service_provider & sp,
          class database_transaction &t,
          const const_data_buffer & channel_id,
          const const_data_buffer &block,
          uint64_t order_no,
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
