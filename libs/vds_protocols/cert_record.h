#ifndef __VDS_PROTOCOLS_CERT_RECORD_H_
#define __VDS_PROTOCOLS_CERT_RECORD_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class cert_record
  {
  public:
    cert_record(
      const std::string & object_name,
      const std::string & cert_body,
      const std::string & cert_key,
      const const_data_buffer & password_hash);


    const std::string & object_name() const { return this->object_name_; }
    const std::string & cert_body() const { return this->cert_body_; }
    const std::string & cert_key() const { return this->cert_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

  private:
    std::string object_name_;
    std::string cert_body_;
    std::string cert_key_;
    const_data_buffer password_hash_;
  };
}

#endif // __VDS_PROTOCOLS_CERT_RECORD_H_
