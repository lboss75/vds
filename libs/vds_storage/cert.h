#ifndef __VDS_STORAGE_CERT_H_
#define __VDS_STORAGE_CERT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_object_id.h"

namespace vds {
  class cert
  {
  public:
    cert(
      const std::string & object_name,
      const full_storage_object_id & object_id,
      const const_data_buffer & password_hash);


    const std::string & object_name() const { return this->object_name_; }
    const full_storage_object_id & object_id() const { return this->object_id_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

  private:
    std::string object_name_;
    full_storage_object_id object_id_;
    const_data_buffer password_hash_;
  };
}

#endif // __VDS_STORAGE_CERT_H_
