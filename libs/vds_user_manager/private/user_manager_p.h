#ifndef __VDS_USER_MANAGER_USER_MANAGER_P_H_
#define __VDS_USER_MANAGER_USER_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "user_manager.h"
#include "security_walker.h"
#include "run_configuration_dbo.h"

namespace vds {
  class _user_manager : public security_walker {
  public:
    _user_manager(
      const guid& id,
      const certificate& device_cert,
      const asymmetric_private_key& device_private_key,
      const uint16_t port);

    member_user get_current_device(
        const service_provider &sp,
        asymmetric_private_key &device_private_key) const;

    user_channel get_channel(const guid &channel_id) const;

    bool validate_and_save(
        const service_provider & sp,
        const std::list<vds::certificate> &cert_chain);
    std::list<user_channel> get_channels() const;

  private:
    guid id_;
    certificate device_cert_;
    asymmetric_private_key device_private_key_;
    uint16_t port_;

    void save_certificate(const service_provider &sp, const certificate &cert);
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_P_H_
