#ifndef __VDS_STORAGE_LOG_RECORDS_H_
#define __VDS_STORAGE_LOG_RECORDS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_log_record
  {
  public:
    std::string fingerprint_;
    std::string signature_;
    std::unique_ptr<json_value> message_;
    
    std::unique_ptr<json_value> serialize();
    void deserialize(json_value * source);
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
    
    std::string certificate_;
    std::string private_key_;
    
    std::unique_ptr<json_value> serialize() const;
    void deserialize(const json_value * source);
  };

  class server_log_batch
  {
  public:
    static const char message_type[];

    int message_id_;
    int previous_message_id_;
    std::unique_ptr<json_array> messages_;

    std::unique_ptr<json_value> serialize();
    void deserialize(json_value * source);
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
