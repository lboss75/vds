#ifndef __VDS_PROTOCOLS_LOG_RECORDS_H_
#define __VDS_PROTOCOLS_LOG_RECORDS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_object_id.h"

namespace vds {
  
  class principal_log_new_object
  {
  public:
    static const char message_type[];
    
    principal_log_new_object(
      const std::shared_ptr<json_value> & source);
    
    principal_log_new_object(
      const guid & owner_principal,
      const guid & index,
      uint32_t lenght,
      const const_data_buffer & hash);
    
    const guid & index() const { return this->index_; }
    uint32_t lenght() const { return this->lenght_; }
    const const_data_buffer & hash() const { return this->hash_; }
    const guid & owner_principal() const { return this->owner_principal_; }
    
    std::shared_ptr<json_value> serialize(bool add_type_property = true) const;
  private:
    guid index_;
    uint32_t lenght_;
    const_data_buffer hash_;
    guid owner_principal_;
  };

  class principal_log_record
  {
  public:
    static const char message_type[];

    typedef guid record_id;

    principal_log_record();

    principal_log_record(
      const principal_log_record & origin);

    principal_log_record(
      const record_id & id,
      const std::list<record_id> & parents,
      const std::shared_ptr<json_value> & message);
    
    principal_log_record(const std::shared_ptr<json_value> & source);

    void reset(
      const record_id & id,
      const std::list<record_id> & parents,
      const std::shared_ptr<json_value> & message);

    const std::shared_ptr<json_value> &  message() const { return this->message_; }

    const record_id & id() const { return this->id_; }
    const std::list<record_id> parents() const { return this->parents_; }
    void add_parent(const guid & index);

    binary_deserializer & deserialize(const service_provider & sp, binary_deserializer & b);
    void serialize(binary_serializer & b) const;
    std::shared_ptr<json_value> serialize(bool add_type_property) const;

  private:
    record_id id_;
    std::list<record_id> parents_;
    std::shared_ptr<json_value> message_;
  };

  class server_log_root_certificate
  {
  public:
    static const char message_type[];

    server_log_root_certificate(
      const std::string & user_cert,
      const std::string & user_private_key,
      const const_data_buffer & password_hash);

    server_log_root_certificate(const std::shared_ptr<json_value> & source);
    
    const std::string & user_cert() const { return this->user_cert_; }
    const std::string & user_private_key() const { return this->user_private_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

    std::shared_ptr<json_value> serialize() const;

  private:
    std::string user_cert_;
    std::string user_private_key_;
    const_data_buffer password_hash_;
  };
  
  class server_log_new_user_certificate
  {
  public:
    static const char message_type[];

    server_log_new_user_certificate(uint64_t user_cert);

    server_log_new_user_certificate(const std::shared_ptr<json_value> & source);

    uint64_t user_cert() const { return this->user_cert_; }

    std::shared_ptr<json_value> serialize() const;

  private:
    uint64_t user_cert_;
  };

  class server_log_new_server
  {
  public:
    static const char message_type[];
    
    server_log_new_server(const std::string & certificate);
    server_log_new_server(const std::shared_ptr<json_value> & source);

    const std::string & certificate () const { return this->certificate_; }
    
    std::shared_ptr<json_value> serialize() const;
    
  private:
    std::string certificate_;
  };

  class server_log_new_endpoint
  {
  public:
    static const char message_type[];

    server_log_new_endpoint(
      const guid & server_id,
      const std::string & addresses);

    server_log_new_endpoint(const std::shared_ptr<json_value> & source);

    const guid & server_id() const { return this->server_id_; }
    const std::string & addresses() const { return this->addresses_; }

    std::shared_ptr<json_value> serialize() const;

  private:
    guid server_id_;
    std::string addresses_;
  };
}

#endif // __VDS_PROTOCOLS_LOG_RECORDS_H_
