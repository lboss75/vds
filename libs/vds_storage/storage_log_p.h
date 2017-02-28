#ifndef __VDS_STORAGE_STORAGE_LOG_P_H_
#define __VDS_STORAGE_STORAGE_LOG_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class storage_log;
  class server_log_root_certificate;
  class cert;
  class node;

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
    certificate * get_cert(const std::string & fingerprint);
    certificate * parse_root_cert(const json_value * value);
    void apply_record(const json_value * value);

    const std::list<cert> & get_certificates() const { return this->certificates_; }
    const std::list<node> & get_nodes() const { return this->nodes_; }

  private:
    storage_log * const owner_;
    logger log_;
    foldername vds_folder_;
    foldername commited_folder_;
    bool is_empty_;

    std::list<cert> certificates_;
    std::list<node> nodes_;

    std::map<std::string, std::unique_ptr<certificate>> loaded_certificates_;

    void process(const server_log_root_certificate & message);
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_P_H_
