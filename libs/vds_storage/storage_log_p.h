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
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_P_H_
