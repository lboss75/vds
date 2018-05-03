#ifndef __VDS_DATA_COIN_COIN_TRANSACTION_PACKAGE_H_
#define __VDS_DATA_COIN_COIN_TRANSACTION_PACKAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"
#include "transactions/payment_transaction.h"
#include "orm/coin_transaction.h"

namespace vds {
  namespace data_coin_private {
    class coin_transaction_package {
    public:

      coin_transaction_package() {

      }

      static coin_transaction_package parse(const const_data_buffer & data);

      const const_data_buffer & id() const {
        return this->id_;
      }

      uint64_t level() const {
        return this->level_;
      }

      const std::list<const_data_buffer> & base_packages() const {
        return this->base_packages_;
      }

      const std::list<data_coin::transactions::payment_transaction> & transactions() const {
        return this->transactions_;
      }

      const const_data_buffer & sign_certificate_thumbprint() const {
        return this->sign_certificate_thumbprint_;
      }

      const const_data_buffer & sign() const {
        return this->sign_;
      }

      static coin_transaction_package load(
          const const_data_buffer & data);

      static uint64_t get_order_no(
          database_transaction &t,
          const const_data_buffer &package_id) {
        data_coin::orm::coin_transaction t1;
        auto st = t.get_reader(
            t1.select(t1.order_no)
                .where(t1.id == base64::from_bytes(package_id)));

        if(!st.execute()){
          throw std::runtime_error("Database is corrupted");
        }

        return t1.order_no.get(st);
      }

    private:
      const_data_buffer id_;
      uint64_t level_;

      std::list<const_data_buffer> base_packages_;
      std::list<data_coin::transactions::payment_transaction> transactions_;
      const_data_buffer sign_certificate_thumbprint_;
      const_data_buffer sign_;
    };
  }
}

#endif //__VDS_DATA_COIN_COIN_TRANSACTION_PACKAGE_H_
