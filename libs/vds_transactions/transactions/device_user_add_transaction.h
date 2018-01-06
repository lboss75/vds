#ifndef __VDS_USER_MANAGER__DEVICE_USER_ADD_TRANSACTION_H_
#define __VDS_USER_MANAGER__DEVICE_USER_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdafx.h>
#include "transaction_log.h"

namespace vds {
  namespace transactions {
    class device_user_add_transaction {
    public:
      static const uint8_t message_id = 'u';

      device_user_add_transaction(
          const guid & user_id,
          const certificate & user_certificate) {

      }

      binary_serializer &serialize(binary_serializer &s) const {
        return s;
      }

    };
  }
}

#endif //__VDS_USER_MANAGER__DEVICE_USER_ADD_TRANSACTION_H_
