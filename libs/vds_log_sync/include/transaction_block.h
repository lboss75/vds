#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include "binary_serialize.h"
#include "database.h"
#include "asymmetriccrypto.h"
#include "channel_messages_walker.h"
#include "transaction_messages_walker.h"

namespace vds {
  namespace transactions {
    class transaction_block {
    public:
      transaction_block() = default;
      transaction_block(transaction_block &&) = default;
      transaction_block(const transaction_block &) = delete;

      transaction_block(
        uint32_t version,
        std::chrono::system_clock::time_point && time_point,
        const_data_buffer && id,
        uint64_t order_no,
        const_data_buffer && write_public_key_id,
        std::set<const_data_buffer> && ancestors,
        const_data_buffer && block_messages,
        const_data_buffer && signature)
      : version_(version),
        time_point_(std::move(time_point)),
        id_(std::move(id)),
        order_no_(order_no),
        write_public_key_id_(std::move(write_public_key_id)),
        ancestors_(std::move(ancestors)),
        block_messages_(std::move(block_messages)),
        signature_(std::move(signature)){
      }

      transaction_block & operator = (const transaction_block &) = delete;
      transaction_block & operator = (transaction_block &&) = default;

      /**
       * \brief Current protocol version
       */
      static constexpr uint32_t CURRENT_VERSION = 0x31564453U;//1VDS

      static expected<transaction_block> create(const const_data_buffer& data);

      static expected<const_data_buffer> build(
        database_transaction& t,
        const const_data_buffer& messages,
        const std::shared_ptr<asymmetric_public_key> & node_public_key,
        const std::shared_ptr<asymmetric_private_key> & node_key);

      const uint32_t & version() const {
        return this->version_;
      }

      std::chrono::system_clock::time_point time_point() const {
        return this->time_point_;
      }


      const const_data_buffer & id() const {
        return this->id_;
      }

      uint64_t order_no() const {
        return this->order_no_;
      }

      const const_data_buffer & write_public_key_id() const {
        return this->write_public_key_id_;
      }

      const std::set<const_data_buffer> & ancestors() const {
        return this->ancestors_;
      }

      const const_data_buffer & block_messages() const {
        return this->block_messages_;
      }

      const const_data_buffer & signature() const {
        return this->signature_;
      }

      expected<bool> validate(const asymmetric_public_key & write_public_key);
      expected<bool> exists(database_transaction& t);

      template <typename... handler_types>
      expected<bool> walk_messages(
        handler_types && ... handlers) const {

        transaction_messages_walker_lambdas<handler_types...> walker(
          std::forward<handler_types>(handlers)...);

        return walker.process(this->block_messages_);
      }

      //static expected<transaction_block_builder> create(
      //  const service_provider* sp,
      //  class vds::database_read_transaction& t);

      //static expected<transaction_block_builder> create(
      //  const service_provider* sp,
      //  class vds::database_read_transaction& t,
      //  const std::set<const_data_buffer>& ancestors);

      //static expected<transaction_block_builder> create(
      //  const service_provider* sp,
      //  class vds::database_read_transaction& t,
      //  const const_data_buffer& data);

      //static transaction_block_builder create_root_block(const service_provider* sp) {
      //  return transaction_block_builder(sp);
      //}
      //expected<const_data_buffer> sign(
      //  const service_provider* sp,
      //  const std::shared_ptr<asymmetric_public_key>& write_public_key,
      //  const std::shared_ptr<asymmetric_private_key>& write_private_key);
      //const service_provider* sp_;
      //std::chrono::system_clock::time_point time_point_;
      //std::set<const_data_buffer> ancestors_;
      //uint64_t order_no_;
      //transaction_block_builder(const service_provider* sp);

      //expected<const_data_buffer> save(
      //  const service_provider* sp,
      //  class vds::database_transaction& t,
      //  const std::shared_ptr<asymmetric_public_key>& write_public_key,
      //  const std::shared_ptr<asymmetric_private_key>& write_private_key);


    private:
      uint32_t version_;
      std::chrono::system_clock::time_point time_point_;
      const_data_buffer id_;
      uint64_t order_no_;
      const_data_buffer write_public_key_id_;
      std::set<const_data_buffer> ancestors_;
      const_data_buffer block_messages_;
      const_data_buffer signature_;
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
