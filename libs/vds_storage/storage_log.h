#ifndef __VDS_STORAGE_STORAGE_LOG_H_
#define __VDS_STORAGE_STORAGE_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _storage_log;
  class endpoint;
  class istorage_log;
  class cert;
  class full_storage_object_id;
  class server_log_record;

  class storage_log
  {
  public:
    storage_log(
      const service_provider & sp,
      const guid & current_server_id,
      const certificate & server_certificate,
      const asymmetric_private_key & server_private_key);
    ~storage_log();

    void start();
    void stop();

  private:
    friend class istorage_log;
    _storage_log * const impl_;
  };
  
  class istorage_log
  {
  public:
    istorage_log(storage_log * owner)
    : owner_(owner)
    {      
    }

    const guid & current_server_id() const;
    const certificate & server_certificate() const;
    const asymmetric_private_key & server_private_key() const;
    
    bool is_empty() const;

    size_t minimal_consensus() const;

    void add_record(const std::string & record);
    void add_to_local_log(const json_value * record);

    size_t new_message_id();
    const std::list<endpoint> & get_endpoints() const;

    vds::async_task<> register_server(const std::string & server_certificate);

    std::unique_ptr<cert> find_cert(const std::string & object_name) const;

    std::unique_ptr<const_data_buffer> get_object(const full_storage_object_id & object_id);

    event_source<const server_log_record & /*record*/, const const_data_buffer & /*signature*/> & new_local_record_event();

    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);

    async_task<> save_file(
      const std::string & version_id,
      const std::string & user_login,
      const std::string & name,
      const filename & tmp_file);
    
    async_task<> reset(
      const certificate & root_certificate,
      const asymmetric_private_key & private_key,
      const std::string & root_password,
      const std::string & address);

    void apply_record(
      const server_log_record & record,
      const const_data_buffer & signature,
      bool check_signature = true);

  private:
    storage_log * owner_;
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_H_
