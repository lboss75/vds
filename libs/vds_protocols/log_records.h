#ifndef __VDS_PROTOCOLS_LOG_RECORDS_H_
#define __VDS_PROTOCOLS_LOG_RECORDS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_object_id.h"
#include "binary_serialize.h"

namespace vds {
  
  class principal_log_new_object
  {
  public:
    static const char message_type[];
    
    principal_log_new_object(
      const std::shared_ptr<json_value> & source);
    
    principal_log_new_object(
      const guid & index,
      uint32_t lenght,
      const const_data_buffer & meta_data);
    
    const guid & index() const { return this->index_; }
    uint32_t lenght() const { return this->lenght_; }
    const const_data_buffer & meta_data() const { return this->meta_data_; }
    
    std::shared_ptr<json_value> serialize(bool add_type_property = true) const;
  private:
    guid index_;
    uint32_t lenght_;
    const_data_buffer meta_data_;
  };
  ////////////////////////////////////////
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
      const guid & principal_id,
      const std::list<record_id> & parents,
      const std::shared_ptr<json_value> & message,
      size_t order_num);
    
    principal_log_record(const std::shared_ptr<json_value> & source);

    void reset(
      const record_id & id,
      const guid & principal_id,
      const std::list<record_id> & parents,
      const std::shared_ptr<json_value> & message,
      size_t order_num);

    const std::shared_ptr<json_value> &  message() const { return this->message_; }

    const record_id & id() const { return this->id_; }
    const guid & principal_id() const { return this->principal_id_; }
    const std::list<record_id> parents() const { return this->parents_; }
    size_t order_num() const { return this->order_num_; }
    void add_parent(const guid & index);

    binary_deserializer & deserialize(const service_provider & sp, binary_deserializer & b);
    void serialize(binary_serializer & b) const;
    std::shared_ptr<json_value> serialize(bool add_type_property = false) const;

  private:
    record_id id_;
    guid principal_id_;
    std::list<record_id> parents_;
    std::shared_ptr<json_value> message_;
    size_t order_num_;
  };

  class server_log_root_certificate
  {
  public:
    static const char message_type[];

    server_log_root_certificate(
      const guid & id,
      const std::string & user_cert,
      const std::string & user_private_key,
      const const_data_buffer & password_hash);

    server_log_root_certificate(const std::shared_ptr<json_value> & source);
    
    const guid & id() const { return this->id_; }
    const std::string & user_cert() const { return this->user_cert_; }
    const std::string & user_private_key() const { return this->user_private_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

    std::shared_ptr<json_value> serialize() const;

  private:
    guid id_;
    std::string user_cert_;
    std::string user_private_key_;
    const_data_buffer password_hash_;
  };
  
  //class server_log_new_user_certificate
  //{
  //public:
  //  static const char message_type[];

  //  server_log_new_user_certificate(uint64_t user_cert);

  //  server_log_new_user_certificate(const std::shared_ptr<json_value> & source);

  //  uint64_t user_cert() const { return this->user_cert_; }

  //  std::shared_ptr<json_value> serialize() const;

  //private:
  //  uint64_t user_cert_;
  //};

  class server_log_new_server
  {
  public:
    static const char message_type[];
    
    server_log_new_server(
      const guid & id,
      const guid & parent_id,
      const std::string & server_cert,
      const std::string & server_private_key,
      const const_data_buffer & password_hash);

    server_log_new_server(
      const std::shared_ptr<json_value> & source);

    const guid & id() const { return this->id_; }
    const guid & parent_id() const { return this->parent_id_; }
    const std::string & server_cert() const { return this->server_cert_; }
    const std::string & server_private_key() const { return this->server_private_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

    std::shared_ptr<json_value> serialize() const;
    
  private:
    guid id_;
    guid parent_id_;
    std::string server_cert_;
    std::string server_private_key_;
    const_data_buffer password_hash_;
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
  /////////////////////////////////////////////////////
  class replica_info
  {
  public:
    replica_info(const std::shared_ptr<json_value> & source);
    std::shared_ptr<json_value> serialize() const;
    
    replica_info(
      size_t size,
      const const_data_buffer & hash)
    : size_(size),
      hash_(hash)
    {
    }
    
    size_t size() const { return this->size_; }
    const const_data_buffer & hash() const { return this->hash_; }
    
  private:
    size_t size_;
    const_data_buffer hash_;
  };
  /////////////////////////////////////////////////////
  class chunk_info
  {
  public:
    chunk_info(const std::shared_ptr<json_value> & source);
    std::shared_ptr<json_value> serialize() const;
    
    chunk_info(
      size_t chunk_index,
      size_t size,
      const const_data_buffer & chunk_hash)
    : chunk_index_(chunk_index),
      size_(size),
      hash_(chunk_hash)
    {
    }
    
    size_t chunk_index() const { return this->chunk_index_; }
    size_t chunk_size() const { return this->size_; }
    const const_data_buffer & chunk_hash() const { return this->hash_; }
    const std::list<replica_info> & replicas() const { return this->replicas_; }
    
    void add_replica(const replica_info & replica)
    {
      this->replicas_.push_back(replica);
    }
    
  private:
    size_t chunk_index_;
    size_t size_;
    const_data_buffer hash_;
    std::list<replica_info> replicas_;
  };

  class principal_log_new_object_map
  {
  public:
    static const char message_type[];
    principal_log_new_object_map(
      const std::shared_ptr<json_value> & source);
    
    std::shared_ptr<json_value> serialize(bool add_type_property = true) const;
    
    principal_log_new_object_map(
      const guid & server_id,
      const guid & object_id,
      uint32_t length,
      const const_data_buffer & object_hash,
      size_t chunk_size)
    : server_id_(server_id),
      object_id_(object_id),
      length_(length),
      hash_(object_hash),
      chunk_size_(chunk_size)
    {
    }
    
    const guid & server_id() const { return this->server_id_; }
    const guid & object_id() const { return this->object_id_; }
    size_t length() const { return this->length_; }
    const const_data_buffer & object_hash() const { return this->hash_; }
    size_t chunk_size() const { return this->chunk_size_; }
    
    std::list<chunk_info> & chunks() { return this->chunks_; }
    
  private:
    guid server_id_;
    guid object_id_;
    size_t length_;
    const_data_buffer hash_;
    size_t chunk_size_;
    std::list<chunk_info> chunks_;
  };
}

#endif // __VDS_PROTOCOLS_LOG_RECORDS_H_
