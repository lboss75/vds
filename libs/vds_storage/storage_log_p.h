#ifndef __VDS_STORAGE_STORAGE_LOG_P_H_
#define __VDS_STORAGE_STORAGE_LOG_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"
#include "server_database.h"
#include "storage_object_id.h"
#include "local_cache.h"

namespace vds {
  class storage_log;
  class server_log_root_certificate;
  class server_log_new_server;
  class server_log_new_endpoint;
  class cert;
  class node;
  class endpoint;
  class ichunk_storage;
  
  class _storage_log
  {
  public:
    _storage_log(
      const service_provider & sp,
      const guid & current_server_id,
      const certificate & server_certificate,
      const asymmetric_private_key & server_private_key,
      storage_log * owner);

    async_task<> reset(
      const certificate & root_certificate,
      const asymmetric_private_key & private_key,
      const std::string & root_password,
      const std::string & addresses);

    void start();
    void stop();

    bool is_empty();
    certificate * get_cert(const std::string & subject);
    certificate * parse_root_cert(const json_value * value);
    void apply_record(const guid & source_server_id, const json_value * value);

    size_t minimal_consensus() const { return this->minimal_consensus_; }

    void add_record(const std::string & record);
    size_t new_message_id();

    vds::async_task<> register_server(const std::string & server_certificate);

    std::unique_ptr<cert> find_cert(const std::string & object_name);
    std::unique_ptr<const_data_buffer> get_object(const full_storage_object_id & object_id);

    event_source<const server_log_record & /*record*/, const const_data_buffer & /*signature*/> & new_local_record_event() { return this->new_local_record_event_; }

    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);

    async_task<> save_file(
      const std::string & version_id,
      const std::string & user_login,
      const std::string & name,
      const filename & tmp_file);

    const guid & current_server_id() const { return this->current_server_id_; }
    const certificate & server_certificate() const { return this->server_certificate_; }
    const asymmetric_private_key & server_private_key() const { return this->current_server_key_; }
    void add_to_local_log(const json_value * record);

    void apply_record(
      const server_log_record & record,
      const const_data_buffer & signature,
      bool check_signature = true);

  private:
    service_provider sp_;
    lazy_service<iserver_database> db_;
    lazy_service<ilocal_cache> local_cache_;
    lazy_service<ichunk_storage> chunk_storage_;
    lazy_service<ichunk_manager> chunk_manager_;

    event_source<const server_log_record & /*record*/, const const_data_buffer & /*signature*/> new_local_record_event_;

    const certificate & server_certificate_;
    const asymmetric_private_key & current_server_key_;

    guid current_server_id_;
    storage_log * const owner_;
    logger log_;
    foldername vds_folder_;

    std::mutex local_log_mutex_;
    uint64_t local_log_index_;

    bool is_empty_;
    size_t minimal_consensus_;

    size_t last_message_id_;

    void process(const guid & source_server_id, const server_log_root_certificate & message);
    void process(const guid & source_server_id, const server_log_new_server & message);
    void process(const guid & source_server_id, const server_log_new_endpoint & message);
    
    async_task<const storage_object_id &>
    save_object(
      const object_container & fc);

    class replica_generator
    {
    public:
      replica_generator(
        uint16_t replica,
        uint16_t min_horcrux
      );
      
      void write(const void * data, size_t len)
      {
        
      }
    };
    
    template<typename context_type>
    class generate_replicas : public dataflow_step<context_type, void(void)>
    {
      using base_class = dataflow_step<context_type, void(void)>;
    public:
      generate_replicas(
        const context_type & context,
        uint16_t min_horcrux,
        uint16_t horcrux_count)
      : base_class(context)
      {
        for(uint16_t i = 0; i < horcrux_count; ++i){
          generators_.push_back(
            std::unique_ptr<replica_generator>(
              new replica_generator(
                i,
                min_horcrux)));
        }
      }
      
      void operator()(const void * data, size_t len)
      {
        for(auto& p : this->generators_){
          p->write(data, len);
        }
        if(0 == len){
          this->next();
        }
        else {
          this->prev();
        }
      }
      
    private:
      std::list<std::unique_ptr<replica_generator>> generators_;
    };
    
    void process_timer_jobs();
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_P_H_
