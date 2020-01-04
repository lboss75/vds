#ifndef __VDS_TRANSACTIONS_TRANSACTION_LACK_OF_FUNDS_H_
#define __VDS_TRANSACTIONS_TRANSACTION_LACK_OF_FUNDS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdexcept>
#include "const_data_buffer.h"

namespace vds {
  namespace transactions {
  class transaction_lack_of_funds : public std::runtime_error {
  public:
    transaction_lack_of_funds(
      const const_data_buffer & source_transaction,
      const const_data_buffer & refer_transaction)
        : std::runtime_error("lack of funds"), source_transaction_(source_transaction), refer_transaction_(refer_transaction) {
    }


    const const_data_buffer & source_transaction() const {
      return this->source_transaction_;
    }

    const const_data_buffer & refer_transaction() const {
      return this->refer_transaction_;
    }

  private:
    const_data_buffer source_transaction_;
    const_data_buffer refer_transaction_;
  };
}
}

#endif //__VDS_TRANSACTIONS_TRANSACTION_LACK_OF_FUNDS_H_
