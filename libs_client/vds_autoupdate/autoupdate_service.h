#ifndef __VDS_VDS_AUTOUPDATE_AUTOUPDATE_SERVICE_H_
#define __VDS_VDS_AUTOUPDATE_AUTOUPDATE_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class user_manager;
}

namespace vds {
  namespace autoupdate {
    class autoupdate_service : public iservice_factory {
    public:
      autoupdate_service();

      expected<void> register_services(service_registrator &) override;
      expected<void> start(const service_provider *) override;
      expected<void> stop() override;
      vds::async_task<vds::expected<void>> prepare_to_stop() override;
      vds::async_task<vds::expected<void>> check_update(const service_provider* sp);

    private:
      timer update_timer_;

      std::shared_ptr<user_manager> user_manager_;
    };
  }
}
#endif // __VDS_VDS_AUTOUPDATE_AUTOUPDATE_SERVICE_H_
