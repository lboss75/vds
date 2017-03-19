#ifndef __VDS_STORAGE_STORAGE_LOG_P_H_
#define __VDS_STORAGE_STORAGE_LOG_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class storage_log;
  class server_log_root_certificate;
  class server_log_new_server;
  class server_log_new_endpoint;
  class cert;
  class node;
  class endpoint;

  class _storage_log
  {
  public:
    _storage_log(
      const service_provider & sp,
      storage_log * owner);

    void reset(
      const std::string & root_password,
      const std::string & addresses);

    void start();
    void stop();

    bool is_empty();
    certificate * get_cert(const std::string & subject);
    certificate * parse_root_cert(const json_value * value);
    void apply_record(const json_value * value);

    const std::list<cert> & get_certificates() const { return this->certificates_; }
    const std::list<node> & get_nodes() const { return this->nodes_; }
    const std::list<endpoint> & get_endpoints() const { return this->endpoints_; }

    size_t minimal_consensus() const { return this->minimal_consensus_; }

    void add_record(const std::string & record);
    size_t new_message_id();

  private:
    storage_log * const owner_;
    logger log_;
    foldername vds_folder_;
    foldername commited_folder_;
    bool is_empty_;
    size_t minimal_consensus_;

    std::list<cert> certificates_;
    std::list<node> nodes_;
    std::list<endpoint> endpoints_;

    std::map<std::string, std::unique_ptr<certificate>> loaded_certificates_;
    certificate_store certificate_store_;

    size_t last_message_id_;

    chunk_storage chunk_storage_;

    void process(const server_log_root_certificate & message);
    void process(const server_log_new_server & message);
    void process(const server_log_new_endpoint & message);
    
    void save_object(const file_container & fc);
    
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
    class generate_replicas : public sequence_step<context_type, void(void)>
    {
      using base_class = sequence_step<context_type, void(void)>;
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
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_P_H_
