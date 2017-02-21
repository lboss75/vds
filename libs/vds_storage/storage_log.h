#ifndef __VDS_STORAGE_STORAGE_LOG_H_
#define __VDS_STORAGE_STORAGE_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class storage_log
  {
  public:
    storage_log();

    void reset(
      const certificate & root_certificate,
      const asymmetric_private_key & private_key,
      const std::string & password);

    void start();

    bool is_empty();
    certificate * get_cert(const std::string & fingerprint);
    certificate * parse_root_cert(const json_value * value);

  private:
    foldername commited_folder_;
    bool is_empty_;

    std::map<std::string, std::unique_ptr<certificate>> certificates_;

  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_H_
