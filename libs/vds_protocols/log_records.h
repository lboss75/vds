#ifndef __VDS_PROTOCOLS_LOG_RECORDS_H_
#define __VDS_PROTOCOLS_LOG_RECORDS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_object_id.h"
#include "binary_serialize.h"
#include "json_object.h"
#include "asymmetriccrypto.h"

namespace vds {
 
  class principal_log_new_object
  {
  public:
    static const uint8_t message_id = 'n';
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
    
    principal_log_new_object(binary_deserializer & b)
    {
      b >> this->index_ >> this->lenght_ >> this->meta_data_;
    }
    
    void serialize(binary_serializer & b) const
    {
      b << this->index_ << this->lenght_ << this->meta_data_;
    }
    
  private:
    guid index_;
    uint32_t lenght_;
    const_data_buffer meta_data_;
  };
  ////////////////////////////////////////
  class principal_log_record
  {
  public:
    static const uint8_t message_id = 'r';
    static const char message_type[];

    typedef guid record_id;

    principal_log_record();

    principal_log_record(
      const principal_log_record & origin);

    principal_log_record(
      const record_id & id,
      const guid & principal_id,
      const guid & member_id,
      const std::list<record_id> & parents,
      const std::shared_ptr<json_value> & message,
      size_t order_num);
    
    principal_log_record(const std::shared_ptr<json_value> & source);

    void reset(
      const record_id & id,
      const guid & principal_id,
      const guid & member_id,
      const std::list<record_id> & parents,
      const std::shared_ptr<json_value> & message,
      size_t order_num);

    const std::shared_ptr<json_value> &  message() const { return this->message_; }

    const record_id & id() const { return this->id_; }
    const guid & principal_id() const { return this->principal_id_; }
    const guid & member_id() const { return this->member_id_; }
    const std::list<record_id> parents() const { return this->parents_; }
    size_t order_num() const { return this->order_num_; }
    void add_parent(const guid & index);

    binary_deserializer & deserialize(const service_provider & sp, binary_deserializer & b);
    void serialize(binary_serializer & b) const;
    std::shared_ptr<json_value> serialize(bool add_type_property = false) const;

    principal_log_record(binary_deserializer & b);
    void serialize(binary_serializer & b);
    
  private:
    record_id id_;
    guid principal_id_;
    guid member_id_;
    std::list<record_id> parents_;
    std::shared_ptr<json_value> message_;
    size_t order_num_;
  };

  class server_log_root_certificate
  {
  public:
    static const uint8_t message_id = 'c';
    static const char message_type[];

    server_log_root_certificate(
      const guid & id,
      const certificate & user_cert,
      const const_data_buffer & user_private_key,
      const const_data_buffer & password_hash);

    server_log_root_certificate(const std::shared_ptr<json_value> & source);
    
    const guid & id() const { return this->id_; }
    const certificate & user_cert() const { return this->user_cert_; }
    const const_data_buffer & user_private_key() const { return this->user_private_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

    std::shared_ptr<json_value> serialize(bool add_type) const;
    
    server_log_root_certificate(binary_deserializer & b)
    {
      const_data_buffer cert_der;
      b >> this->id_ >> cert_der >> this->user_private_key_ >> this->password_hash_;
      this->user_cert_ = certificate::parse_der(cert_der);
    }
    
    void serialize(binary_serializer & b) const
    {
      b << this->id_ << this->user_cert_.der() << this->user_private_key_ << this->password_hash_;
    }

  private:
    guid id_;
    certificate user_cert_;
    const_data_buffer user_private_key_;
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
    static const uint8_t message_id = 's';
    static const char message_type[];
    
    server_log_new_server(
      const guid & id,
      const guid & parent_id,
      const certificate & server_cert,
      const const_data_buffer & server_private_key,
      const const_data_buffer & password_hash);

    server_log_new_server(
      const std::shared_ptr<json_value> & source);

    const guid & id() const { return this->id_; }
    const guid & parent_id() const { return this->parent_id_; }
    const certificate & server_cert() const { return this->server_cert_; }
    const const_data_buffer & server_private_key() const { return this->server_private_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

    std::shared_ptr<json_value> serialize(bool add_type) const;
    
    server_log_new_server(binary_deserializer & b)
    {
      const_data_buffer server_der;
      b >> this->id_ >> this->parent_id_ >> server_der >> this->server_private_key_  >> this->password_hash_;
      
      this->server_cert_ = certificate::parse_der(server_der);
    }
    
    void serialize(binary_serializer & b) const
    {
      b << this->id_ << this->parent_id_ << this->server_cert_.der() << this->server_private_key_  << this->password_hash_;
    }
    
  private:
    guid id_;
    guid parent_id_;
    certificate server_cert_;
    const_data_buffer server_private_key_;
    const_data_buffer password_hash_;
  };

  class server_log_new_endpoint
  {
  public:
    static const uint8_t message_id = 'e';
    static const char message_type[];

    server_log_new_endpoint(
      const guid & server_id,
      const std::string & addresses);

    server_log_new_endpoint(const std::shared_ptr<json_value> & source);

    const guid & server_id() const { return this->server_id_; }
    const std::string & addresses() const { return this->addresses_; }

    std::shared_ptr<json_value> serialize(bool add_type) const;

    server_log_new_endpoint(binary_deserializer & b)
    {
      b >> this->server_id_ >> this->addresses_;
    }
    void serialize(binary_serializer & b) const
    {
      b << this->server_id_ << this->addresses_;
    }
  private:
    guid server_id_;
    std::string addresses_;
  };
  /////////////////////////////////////////////////////
  class principal_log_new_chunk
  {
  public:
    static const uint8_t message_id = 't';
    
    static const char message_type[];
    principal_log_new_chunk(const std::shared_ptr<json_value> & source);
    std::shared_ptr<json_value> serialize(bool add_type) const;
    
    principal_log_new_chunk(
      const guid & server_id,
      size_t chunk_index,
      const guid & object_id,
      size_t size,
      const const_data_buffer & chunk_hash)
    : server_id_(server_id),
      object_id_(object_id),
      chunk_index_(chunk_index),
      size_(size),
      hash_(chunk_hash)
    {
    }
    
    const guid & server_id() const { return this->server_id_; }
    const guid & object_id() const { return this->object_id_; }
    size_t chunk_index() const { return this->chunk_index_; }
    size_t chunk_size() const { return this->size_; }
    size_t replica_size() const;
    const const_data_buffer & chunk_hash() const { return this->hash_; }
    
    principal_log_new_chunk(binary_deserializer & b)
    {
      b >> this->server_id_ >> this->object_id_ >> this->chunk_index_ >> this->size_ >> this->hash_;
    }
    
    void serialize(binary_serializer & b) const
    {
      b << this->server_id_ << this->object_id_ << this->chunk_index_ << this->size_ << this->hash_;
    }

  private:
    guid server_id_;
    guid object_id_;
    size_t chunk_index_;
    size_t size_;
    const_data_buffer hash_;
  };
  ////////////////////////////////////
  class principal_log_new_replica
  {
  public:
    static const uint8_t message_id = 'r';

    static const char message_type[];
    principal_log_new_replica(const std::shared_ptr<json_value> & source);
    std::shared_ptr<json_value> serialize(bool add_type) const;
    
    principal_log_new_replica(
      const guid & server_id,
      size_t chunk_index,
      const guid & object_id,
      size_t index,
      size_t replica_size,
      const const_data_buffer & replica_hash)
    : server_id_(server_id),
      object_id_(object_id),
      chunk_index_(chunk_index),
      index_(index),
      replica_size_(replica_size),
      replica_hash_(replica_hash)
    {
    }
    
    const guid & server_id() const { return this->server_id_; }
    const guid & object_id() const { return this->object_id_; }
    size_t chunk_index() const { return this->chunk_index_; }
    size_t index() const { return this->index_; }
    size_t replica_size() const { return this->replica_size_; }
    const const_data_buffer & replica_hash() const { return this->replica_hash_; }
    
    principal_log_new_replica(binary_deserializer & b)
    {
      b >> this->server_id_ >> this->object_id_ >> this->chunk_index_ >> this->index_ >> this->replica_size_ >> this->replica_hash_;
    }
    
    void serialize(binary_serializer & b) const
    {
      b << this->server_id_ << this->object_id_ << this->chunk_index_ << this->index_ << this->replica_size_ << this->replica_hash_;
    }

  private:
    guid server_id_;
    guid object_id_;
    size_t chunk_index_;
    size_t index_;
    size_t replica_size_;
    const_data_buffer replica_hash_;
  };

  class principal_log_store_replica
  {
  public:
    static const uint8_t message_id = 'k';

    static const char message_type[];
    principal_log_store_replica(const std::shared_ptr<json_value> & source);
    std::shared_ptr<json_value> serialize(bool add_type) const;

    principal_log_store_replica(
      const guid & server_id,
      size_t chunk_index,
      size_t replica_index)
      : server_id_(server_id),
      chunk_index_(chunk_index),
      replica_index_(replica_index)
    {
    }

    const guid & server_id() const { return this->server_id_; }
    size_t chunk_index() const { return this->chunk_index_; }
    size_t replica_index() const { return this->replica_index_; }

    principal_log_store_replica(binary_deserializer & b)
    {
      b >> this->server_id_ >> this->chunk_index_ >> this->replica_index_;
    }

    void serialize(binary_serializer & b) const
    {
      b << this->server_id_ << this->chunk_index_ << this->replica_index_;
    }

  private:
    guid server_id_;
    size_t chunk_index_;
    size_t replica_index_;
  };

  class principal_log_new_local_user
  {
  public:
    static const uint8_t message_id = 'l';
    static const char message_type[];

    principal_log_new_local_user(
      const guid & member_id,
      const guid & parent_id,
      const certificate & member_cert,
      const std::string & member_name)
    : member_id_(member_id),
      parent_id_(parent_id),
      member_cert_(member_cert),
      member_name_(member_name)
    {
    }

    principal_log_new_local_user(const std::shared_ptr<json_value> & source)
    {
      auto s = std::dynamic_pointer_cast<json_object>(source);
      if (s) {
        s->get_property("m", this->member_id_);
        s->get_property("p", this->parent_id_);

        std::string member_cert;
        s->get_property("c", member_cert);
        this->member_cert_ = certificate::parse(member_cert);

        s->get_property("n", this->member_name_);
      }
    }

    const guid & member_id() const { return this->member_id_; }
    const guid & parent_id() const { return this->parent_id_; }
    const certificate & member_cert() const { return this->member_cert_; }
    const std::string & member_name() const { return this->member_name_; }

    std::shared_ptr<json_value> serialize(bool add_type) const
    {
      auto result = std::make_shared<json_object>();
      if (add_type) {
        result->add_property("$t", message_type);
      }
      result->add_property("m", this->member_id_);
      result->add_property("p", this->parent_id_);
      result->add_property("c", this->member_cert_.str());
      result->add_property("n", this->member_name_);
      return result;
    }

    principal_log_new_local_user(binary_deserializer & b)
    {
      const_data_buffer member_cert;
      b >> this->member_id_ >> this->parent_id_ >> member_cert >> this->member_name_;

      this->member_cert_ = certificate::parse_der(member_cert);
    }

    void serialize(binary_serializer & b) const
    {
      b << this->member_id_ << this->parent_id_ << this->member_cert_.der() << this->member_name_;
    }

  private:
    guid member_id_;
    guid parent_id_;
    certificate member_cert_;
    std::string member_name_;
  };
//   class principal_log_new_object_map
//   {
//   public:
//     static const char message_type[];
//     principal_log_new_object_map(
//       const std::shared_ptr<json_value> & source);
//     
//     std::shared_ptr<json_value> serialize(bool add_type_property = true) const;
//     
//     principal_log_new_object_map(
//       const guid & server_id,
//       const guid & object_id,
//       uint32_t length,
//       const const_data_buffer & object_hash,
//       size_t min_chunk,
//       size_t max_chunk)
//     : server_id_(server_id),
//       object_id_(object_id),
//       length_(length),
//       hash_(object_hash),
//       min_chunk_(min_chunk),
//       max_chunk_(max_chunk)
//     {
//     }
//     
//     const guid & server_id() const { return this->server_id_; }
//     const guid & object_id() const { return this->object_id_; }
//     size_t length() const { return this->length_; }
//     const const_data_buffer & object_hash() const { return this->hash_; }
//     size_t min_chunk() const { return this->min_chunk_; }
//     size_t max_chunk() const { return this->max_chunk_; }
//     
//   private:
//     guid server_id_;
//     guid object_id_;
//     size_t length_;
//     const_data_buffer hash_;
//     size_t min_chunk_;
//     size_t max_chunk_;
//   };
  
  template<typename target_type>
  inline void parse_message(const std::shared_ptr<json_value> & xml, const target_type & target)
  {
    auto obj = std::dynamic_pointer_cast<json_object>(xml);
    if(!obj){
      throw std::runtime_error("Unexpected messsage");
    }
    
    std::string message_type;
    if (!obj->get_property("$t", message_type)) {
      throw std::runtime_error("Unexpected messsage");
    }
  
    if (principal_log_new_object::message_type == message_type) {
      target(principal_log_new_object(obj));
    }
    else if (principal_log_new_chunk::message_type == message_type) {
      target(principal_log_new_chunk(obj));
    }
    else if (principal_log_new_replica::message_type == message_type) {
      target(principal_log_new_replica(obj));
    }
    else if(server_log_root_certificate::message_type == message_type){
      target(server_log_root_certificate(obj));
    }
    else if(server_log_new_server::message_type == message_type){
      target(server_log_new_server(obj));
    }
    else if(server_log_new_endpoint::message_type == message_type){
      target(server_log_new_endpoint(obj));
    }
    else if (principal_log_store_replica::message_type == message_type) {
      target(principal_log_store_replica(obj));
    }
    else {
      throw std::runtime_error("Unexpected messsage type " + message_type);
    }
  };
  
  template<typename target_type>
  inline void parse_message(binary_deserializer & b, const target_type & target)
  {
    uint8_t message_id;
    b >> message_id;
    
    switch(message_id){
      case principal_log_new_object::message_id:
        target(principal_log_new_object(b));
        break;
    
      case principal_log_new_chunk::message_id:
        target(principal_log_new_chunk(b));
        break;
    
      case principal_log_new_replica::message_id:
        target(principal_log_new_replica(b));
        break;
    
      case server_log_root_certificate::message_id:
        target(server_log_root_certificate(b));
        break;
    
      case server_log_new_server::message_id:
        target(server_log_new_server(b));
        break;
    
      case server_log_new_endpoint::message_id:
        target(server_log_new_endpoint(b));
        break;
        
      case principal_log_store_replica::message_id:
        target(principal_log_store_replica(b));
        break;

      default:
        throw std::runtime_error("Unexpected messsage type " + std::to_string(message_id));
    }
  };
  
  class message_binary_serializer
  {
  public:
    message_binary_serializer(binary_serializer & b)
    : b_(b)
    {
    }
    
    template<typename record_type>
    void operator()(const record_type & obj) const
    {
      this->b_ << record_type::message_id;
      obj.serialize(this->b_);
    }

  private:
    binary_serializer & b_;
  };
  
  class message_xml_serializer
  {
  public:
    message_xml_serializer(std::shared_ptr<json_value> & target)
    : target_(target)
    {
    }
    
    template<typename record_type>
    void operator()(const record_type & obj) const
    {
      this->target_ = obj.serialize(true);
    }

  private:
    std::shared_ptr<json_value> & target_;
  };
}

#endif // __VDS_PROTOCOLS_LOG_RECORDS_H_
