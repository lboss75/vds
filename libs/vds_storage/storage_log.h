#ifndef __VDS_STORAGE_STORAGE_LOG_H_
#define __VDS_STORAGE_STORAGE_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class server_log_root_certificate;

  class storage_log
  {
  public:
    storage_log(const service_provider & sp);

    void reset(
      const std::string & root_password,
      const std::string & addresses);

    void start();

    bool is_empty();
    certificate * get_cert(const std::string & fingerprint);
    certificate * parse_root_cert(const json_value * value);
    void apply_record(const json_value * value);

    bool get_cert_and_key(
      const std::string & object_name,
      const std::string & password_hash,
      std::string & cert_body,
      std::string & key_body);

  private:
    logger log_;
    foldername vds_folder_;
    foldername commited_folder_;
    bool is_empty_;

    std::map<std::string, std::unique_ptr<certificate>> certificates_;

    struct cert_and_key
    {
      std::string cert_body;
      std::string key_body;
    };

    std::map<std::string, std::map<std::string, cert_and_key>> cert_and_keys_;

    void process(const server_log_root_certificate & message);
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_H_
