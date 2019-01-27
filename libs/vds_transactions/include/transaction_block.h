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
        std::string && write_cert_id,
        std::set<const_data_buffer> && ancestors,
        const_data_buffer && block_messages,
        const_data_buffer && signature)
      : version_(version),
        time_point_(std::move(time_point)),
        id_(std::move(id)),
        order_no_(order_no),
        write_cert_id_(std::move(write_cert_id)),
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

      static expected<transaction_block> create(const const_data_buffer &data) {

        GET_EXPECTED(id, hash::signature(hash::sha256(), data));

        binary_deserializer s(data);
        uint32_t version;
        CHECK_EXPECTED(s >> version);

        if (version != CURRENT_VERSION) {
          return vds::make_unexpected<std::runtime_error>("Invalid block version");
        }

        uint64_t time_point;
        CHECK_EXPECTED(s >> time_point);

        uint64_t order_no;
        CHECK_EXPECTED(s >> order_no);

        std::string write_cert_id;
        CHECK_EXPECTED(s >> write_cert_id);

        std::set<const_data_buffer> ancestors;
        CHECK_EXPECTED(s >> ancestors);

        const_data_buffer block_messages;
        CHECK_EXPECTED(s >> block_messages);

        const_data_buffer signature;
        CHECK_EXPECTED(s >> signature);

        return transaction_block(
          version,
          std::chrono::system_clock::from_time_t(time_point),
          std::move(id),
          order_no,
          std::move(write_cert_id),
          std::move(ancestors),
          std::move(block_messages),
          std::move(signature));
      }

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

      const std::string & write_cert_id() const {
        return this->write_cert_id_;
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

      expected<bool> validate(const certificate& write_cert);
      expected<bool> exists(database_transaction& t);

      template <typename... handler_types>
      expected<bool> walk_messages(
        handler_types && ... handlers) const {

        transaction_messages_walker_lambdas<handler_types...> walker(
          std::forward<handler_types>(handlers)...);

        return walker.process(this->block_messages_);
      }

    private:
      uint32_t version_;
      std::chrono::system_clock::time_point time_point_;
      const_data_buffer id_;
      uint64_t order_no_;
      std::string write_cert_id_;
      std::set<const_data_buffer> ancestors_;
      const_data_buffer block_messages_;
      const_data_buffer signature_;
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
