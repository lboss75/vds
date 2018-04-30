#ifndef __VDS_DATA_COIN_COIN_TRANSACTION_PACKAGE_H_
#define __VDS_DATA_COIN_COIN_TRANSACTION_PACKAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <database_orm.h>

namespace vds {
  namespace data_coin {
    class coin_transaction_package {
    public:

      enum class base_package_state {
        approved,
        rejected
      };

      struct base_package_t {
        base_package_state state;
        const_data_buffer package_id;
      };

      coin_transaction_package() {

      }

      const std::list<base_package_t> & base_packages() const {
        return this->base_packages_;
      }

      const const_data_buffer & id() const {
        return this->id_;
      }

      uint64_t level() const {
        return this->level_;
      }

    private:
      const_data_buffer id_;
      uint64_t level_;

      std::list<base_package_t> base_packages_;
      const_data_buffer sign_certificate_thumbprint_;
      const_data_buffer sign_;
      const_data_buffer data_;
    };
  }
}

#endif //__VDS_DATA_COIN_COIN_TRANSACTION_PACKAGE_H_
