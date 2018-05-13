#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include <map>
#include <private/stdafx.h>
#include "binary_serialize.h"
#include "database.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"

namespace vds {
  namespace transactions {
    class transaction_block {
    public:
      transaction_block(const const_data_buffer &data){
        binary_deserializer s(data);
        s
            >> this->order_no_
            >> this->write_cert_id_
            >> this->ancestors_
            >> this->block_messages_
            >> this->certificates_
            >> this->signature_;
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

      const std::list<certificate> & certificates() const {
        return this->certificates_;
      }

      const const_data_buffer & signature() const {
        return this->signature_;
      }

    private:
      uint64_t order_no_;
      std::string write_cert_id_;
      std::set<const_data_buffer> ancestors_;
      const_data_buffer block_messages_;
      std::list<certificate> certificates_;
      const_data_buffer signature_;
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
