#ifndef __VDS_STORAGE_LOG_RECORDS_H_
#define __VDS_STORAGE_LOG_RECORDS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_object_id.h"

namespace vds {
  
  class server_log_batch
  {
  public:
    static const char message_type[];

    server_log_batch(size_t message_id);
    server_log_batch(const json_value * source);

    size_t message_id() const { return this->message_id_; }
    const json_array * get_messages() const { return this->messages_.get(); }

    void add(std::unique_ptr<json_value> && item);

    std::unique_ptr<json_value> serialize() const;

  private:
    size_t message_id_;
    std::unique_ptr<json_array> messages_;
  };
  
  class server_log_new_object
  {
  public:
    static const char message_type[];
    
    server_log_new_object(
      const json_value * source);
    
    server_log_new_object(
      uint64_t index,
      uint32_t original_lenght,
      const data_buffer & original_hash,
      uint32_t target_lenght,
      const data_buffer & target_hash);
    
    uint64_t index() const { return this->index_; }
    uint32_t original_lenght() const { return this->original_lenght_; }
    const data_buffer & original_hash() const { return this->original_hash_; }
    uint32_t target_lenght() const { return this->target_lenght_; }
    const data_buffer & target_hash() const { return this->target_hash_; }
    
    std::unique_ptr<json_value> serialize(bool add_type_property = true) const;
  private:
    uint64_t index_;
    uint32_t original_lenght_;
    data_buffer original_hash_;
    uint32_t target_lenght_;
    data_buffer target_hash_;
  };

  class server_log_file_map
  {
  public:
    static const char message_type[];

    server_log_file_map(
      const std::string & version_id,
      const std::string & user_login,
      const std::string & name);

    server_log_file_map(
      const json_value * source);

    void add(const server_log_new_object & item);
    std::unique_ptr<json_value> serialize(bool add_type_property = true) const;

    const std::string & version_id() const { return this->version_id_; }
    const std::string & user_login() const { return this->user_login_; }
    const std::string & name() const { return this->name_; }
    const std::list<server_log_new_object> & items() const { return this->items_; }
    
  private:
    std::string version_id_;
    std::string user_login_;
    std::string name_;
    std::list<server_log_new_object> items_;
  };


  class server_log_sign
  {
  public:
    server_log_sign(
      const std::string & subject,
      const data_buffer & signature);

    server_log_sign(const json_value * source);

    const std::string & subject() const { return this->subject_; }
    const data_buffer & signature() const { return this->signature_; }

    std::unique_ptr<json_value> serialize() const;

  private:
    std::string subject_;
    data_buffer signature_;
  };

  class server_log_record
  {
  public:
    static const char message_type[];

    server_log_record(std::unique_ptr<server_log_batch> && message);
    server_log_record(const json_value * source);

    const server_log_batch * message() const { return this->message_.get(); }
    const std::list<server_log_sign> & signatures() const { return this->signatures_; }

    void add_signature(
      const std::string & subject,
      const data_buffer & signature);

    std::unique_ptr<json_value> serialize(bool add_type_property) const;

  private:
    std::unique_ptr<server_log_batch> message_;
    std::list<server_log_sign> signatures_;
  };
  
  class server_log_root_certificate
  {
  public:
    static const char message_type[];

    server_log_root_certificate(
      const storage_object_id & user_cert,
      const data_buffer & password_hash);

    server_log_root_certificate(const json_value * source);
    
    const storage_object_id & user_cert() const { return this->user_cert_; }
    const data_buffer & password_hash() const { return this->password_hash_; }

    std::unique_ptr<json_value> serialize() const;

  private:
    storage_object_id user_cert_;
    data_buffer password_hash_;
  };
  
  class server_log_new_user_certificate
  {
  public:
    static const char message_type[];

    server_log_new_user_certificate(uint64_t user_cert);

    server_log_new_user_certificate(const json_value * source);

    uint64_t user_cert() const { return this->user_cert_; }

    std::unique_ptr<json_value> serialize() const;

  private:
    uint64_t user_cert_;
  };

  class server_log_new_server
  {
  public:
    static const char message_type[];
    
    server_log_new_server(const storage_object_id & cert_id_);
    server_log_new_server(const json_value * source);

    const storage_object_id & cert_id() const { return this->cert_id_; }
    
    std::unique_ptr<json_value> serialize() const;
    
  private:
    storage_object_id cert_id_;
  };

  class server_log_new_endpoint
  {
  public:
    static const char message_type[];

    server_log_new_endpoint(
      const guid & server_id,
      const std::string & addresses);

    server_log_new_endpoint(const json_value * source);

    const guid & server_id() const { return this->server_id_; }
    const std::string & addresses() const { return this->addresses_; }

    std::unique_ptr<json_value> serialize() const;

  private:
    guid server_id_;
    std::string addresses_;
  };
  //
  class server_log_put_file
  {
  public:
    static const char message_type[];

    class file_part
    {
    public:
      file_part(
        uint64_t index,
        uint32_t length,
        const data_buffer & signature);

      uint64_t index() const { return this->index_; }
      uint32_t length() const { return this->length_; }
      const data_buffer & signature() const { return this->signature_; }

    private:
      uint64_t index_;
      uint32_t length_;
      data_buffer signature_;
    };

    const std::string & user_login() const { return this->user_login_; }
    const std::string & name() const { return this->name_; }
    const std::list<file_part> & parts() const { return this->parts_; }

  private:
    std::string user_login_;
    std::string name_;
    std::list<file_part> parts_;
  };

  class server_log_create_chunk
  {
  public:

  private:
    uint64_t chunk_index_;
    std::list<uint64_t> parts_;
  };

  class server_log_copy_chunk_replica
  {
  public:

  private:
    guid source_id_;
    uint64_t chunk_index_;
    uint64_t chunk_replica_;
  };

  class server_log_build_chunk
  {
  public:

  private:
    guid source_id_;
    uint64_t chunk_index_;
    uint64_t chunk_replica_;
  };

  class server_log_delete_chunk
  {
  public:

  private:
    guid source_id_;
    uint64_t chunk_index_;
    uint64_t chunk_replica_;
  };

}

#endif // __VDS_STORAGE_LOG_RECORDS_H_
