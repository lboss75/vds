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
    
    std::unique_ptr<json_object> serialize();
    void deserialize(json_value * source);
  };
  
  class server_log_root_certificate
  {
  public:
    static const char message_type[];
    
    std::string certificate_;
    std::string private_key_;
    
    std::unique_ptr<json_object> serialize() const;
    void deserialize(const json_value * source);
  };
  
  class server_log_new_user_certificate
  {
  public:
    static const char message_type[];
    
    std::string certificate_;
    std::string private_key_;
    
    std::unique_ptr<json_object> serialize() const;
    void deserialize(const json_value * source);
  };

  class server_log_batch
  {
  public:
    static const char message_type[];

    int message_id_;
    int previous_message_id_;
    std::unique_ptr<json_array> messages_;

    std::unique_ptr<json_object> serialize();
    void deserialize(json_value * source);
  };

  class server_log_new_server
  {
  public:
    static const char message_type[];

    std::string certificate_;
    std::string addresses_;

    std::unique_ptr<json_object> serialize() const;
    void deserialize(const json_value * source);
  };
}

#endif // __VDS_STORAGE_LOG_RECORDS_H_
