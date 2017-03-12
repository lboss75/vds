#ifndef __VDS_STORAGE_LOG_RECORDS_H_
#define __VDS_STORAGE_LOG_RECORDS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

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
      const std::string & certificate,
      const std::string & private_key,
      const std::string & password_hash);

    server_log_root_certificate(const json_value * source);
    
    const std::string & certificate() const { return this->certificate_; }
    const std::string & private_key() const { return this->private_key_; }
    const std::string & password_hash() const { return this->password_hash_; }

    std::unique_ptr<json_value> serialize() const;

  private:
    std::string certificate_;
    std::string private_key_;
    std::string password_hash_;
  };
  
  class server_log_new_user_certificate
  {
  public:
    static const char message_type[];

    server_log_new_user_certificate(
      const std::string & certificate,
      const std::string & private_key);

    server_log_new_user_certificate(const json_value * source);

    const std::string & certificate() const { return this->certificate_; }
    const std::string & private_key() const { return this->private_key_; }

    std::unique_ptr<json_value> serialize() const;

  private:
    std::string certificate_;
    std::string private_key_;
  };

  class server_log_new_server
  {
  public:
    static const char message_type[];
    
    server_log_new_server(
      const std::string & certificate);
    
    server_log_new_server(const json_value * source);

    const std::string & certificate() const { return this->certificate_; }
    
    std::unique_ptr<json_value> serialize() const;
    
  private:
    std::string certificate_;    
  };

  class server_log_new_endpoint
  {
  public:
    static const char message_type[];

    server_log_new_endpoint(
      const std::string & addresses);

    server_log_new_endpoint(const json_value * source);

    const std::string & addresses() const { return this->addresses_; }

    std::unique_ptr<json_value> serialize() const;

  private:
    std::string addresses_;
  };
}

#endif // __VDS_STORAGE_LOG_RECORDS_H_
