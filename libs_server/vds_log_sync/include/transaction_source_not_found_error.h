#ifndef __VDS_TRANSACTIONS_TRANSACTION_SOURCE_NOT_FOUND_ERROR_H_
#define __VDS_TRANSACTIONS_TRANSACTION_SOURCE_NOT_FOUND_ERROR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdexcept>
#include "const_data_buffer.h"

namespace vds {
  namespace transactions {
    class transaction_source_not_found_error : public std::runtime_error {
    public:
      transaction_source_not_found_error(
        const const_data_buffer & source_id,
        const const_data_buffer & refer_transaction)
        : std::runtime_error("Source is not found"), source_id_(source_id), refer_transaction_(refer_transaction) {

      }

      const const_data_buffer & source_id() const {
        return this->source_id_;
      }

      const const_data_buffer & refer_transaction() const {
        return this->refer_transaction_;
      }

    private:
      const_data_buffer source_id_;
      const_data_buffer refer_transaction_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_TRANSACTION_SOURCE_NOT_FOUND_ERROR_H_
