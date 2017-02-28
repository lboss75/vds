#ifndef __VDS_STORAGE_CERT_H_
#define __VDS_STORAGE_CERT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_cursor.h"

namespace vds {
  class istorage;

  class cert
  {
  public:
    cert(
      const std::string & object_name,
      const std::string & certificate,
      const std::string & private_key,
      const std::string & password_hash);


    const std::string & object_name() const { return this->object_name_; }
    const std::string & certificate() const { return this->certificate_; }
    const std::string & private_key() const { return this->private_key_; }
    const std::string & password_hash() const { return this->password_hash_; }

  private:
    std::string object_name_;
    std::string certificate_;
    std::string private_key_;
    std::string password_hash_;
  };

  template <>
  class storage_cursor<cert> : public _simple_storage_cursor<cert>
  {
  public:
    storage_cursor(const istorage & storage);
  };

}

#endif // __VDS_STORAGE_CERT_H_
