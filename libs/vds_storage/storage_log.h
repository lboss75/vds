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
    storage_log(const service_provider & sp);

    void reset(
      const std::string & root_password,
      const std::string & addresses);

    void start();

    bool is_empty();
    certificate * get_cert(const std::string & fingerprint);
    certificate * parse_root_cert(const json_value * value);
    void apply_record(const json_value * value);

  private:
    logger log_;
    foldername vds_folder_;
    foldername commited_folder_;
    bool is_empty_;

    std::map<std::string, std::unique_ptr<certificate>> certificates_;

  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_H_
