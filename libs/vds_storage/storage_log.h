#ifndef __VDS_STORAGE_STORAGE_LOG_H_
#define __VDS_STORAGE_STORAGE_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _storage_log;
  class endpoint;
  class istorage;
  class cert;
  class full_storage_object_id;

  class storage_log
  {
  public:
    storage_log(
      const service_provider & sp,
      const guid & current_server_id,
      const certificate & server_certificate,
      const asymmetric_private_key & server_private_key);
    ~storage_log();

    async_task<void(void)> reset(
      const certificate & root_certificate,
      const asymmetric_private_key & private_key,
      const std::string & root_password,
      const std::string & address);

    void start();
    void stop();

    bool is_empty() const;

    _storage_log * operator -> () const;

    size_t minimal_consensus() const;

    void add_record(const std::string & record);

    size_t new_message_id();
    const std::list<endpoint> & get_endpoints() const;

    void register_server(const std::string & server_certificate);

    std::unique_ptr<cert> find_cert(const std::string & object_name) const;

    std::unique_ptr<data_buffer> get_object(const full_storage_object_id & object_id);
    
    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);

    void save_file(
      const std::string & user_login,
      const filename & tmp_file);
  private:
    std::unique_ptr<_storage_log> impl_;
  };
  
  class istorage
  {
  public:
    istorage(storage_log * owner)
    : owner_(owner)
    {      
    }
    
    storage_log & get_storage_log() const { return *this->owner_; }
    
  private:
    storage_log * owner_;
    
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_H_
