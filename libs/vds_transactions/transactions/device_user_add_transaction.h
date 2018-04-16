#ifndef __VDS_USER_MANAGER__DEVICE_USER_ADD_TRANSACTION_H_
#define __VDS_USER_MANAGER__DEVICE_USER_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdafx.h>
#include "transaction_id.h"

namespace vds {
  namespace transactions {
  //  class device_user_add_transaction {
  //  public:
  //    static const uint8_t message_id = (uint8_t)transaction_id::device_user_add_transaction;

  //    device_user_add_transaction(
  //        const guid & user_id,
  //        const certificate & user_certificate)
  //    : user_id_(user_id), user_certificate_(user_certificate){
  //    }

  //    device_user_add_transaction(binary_deserializer & s){
  //      const_data_buffer user_certificate_der;
  //      s >> this->user_id_ >> user_certificate_der;
  //      this->user_certificate_ = certificate::parse_der(user_certificate_der);
  //    }

  //    binary_serializer &serialize(binary_serializer &s) const {
  //      return s << this->user_id_ << this->user_certificate_.der();
  //    }

  //    void apply(
  //        const service_provider & sp,
  //        database_transaction & t) const{

  //    }

  //  private:
  //    certificate user_certificate_;
  //  };
  }
}

#endif //__VDS_USER_MANAGER__DEVICE_USER_ADD_TRANSACTION_H_
