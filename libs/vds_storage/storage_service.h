#ifndef __VDS_STORAGE_STORAGE_SERVICE_H_
#define __VDS_STORAGE_STORAGE_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class storage_service;
  class storage_log;

  class istorage
  {
  public:
    istorage(storage_service * owner);

    storage_log & get_storage_log() const;

  private:
    storage_service * owner_;
  };

  class storage_service : public iservice
  {
  public:
    storage_service();
    ~storage_service();

    // Inherited via iservice
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

  private:
    friend class istorage;
    std::unique_ptr<storage_log> storage_log_;
  };
}

#endif // __VDS_STORAGE_STORAGE_SERVICE_H_
