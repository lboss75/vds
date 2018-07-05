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

      /**
       * \brief Current protocol version
       */
      static constexpr uint32_t CURRENT_VERSION = 0x31564453U;//1VDS

      transaction_block(const const_data_buffer &data)
      : id_(hash::signature(hash::sha256(), data)){
        binary_deserializer s(data);
        s
          >> this->version_;

        if(this->version_ != CURRENT_VERSION) {
          throw std::runtime_error("Invalid block version");
        }

        uint64_t time_point;
        s
          >> time_point
          >> this->order_no_
          >> this->write_cert_id_
          >> this->ancestors_
          >> this->block_messages_
          >> this->signature_;

        this->time_point_ = std::chrono::system_clock::from_time_t(time_point);
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

      bool validate(const certificate& write_cert);
      bool exists(database_transaction& t);

      template <typename... handler_types>
      bool walk_messages(
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
