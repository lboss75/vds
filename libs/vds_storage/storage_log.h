#ifndef __VDS_STORAGE_STORAGE_LOG_H_
#define __VDS_STORAGE_STORAGE_LOG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _storage_log;

  class storage_log
  {
  public:
    storage_log(const service_provider & sp);
    ~storage_log();

    void reset(
      const std::string & root_password,
      const std::string & addresses);

    void start();
    void stop();

    bool is_empty() const;

    _storage_log * operator -> () const;

  private:
    std::unique_ptr<_storage_log> impl_;
  };
}

#endif // __VDS_STORAGE_STORAGE_LOG_H_
