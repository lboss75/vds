#ifndef __VDS_USER_MANAGER_SECURITY_WALKER_H_
#define __VDS_USER_MANAGER_SECURITY_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include "guid.h"
#include "asymmetriccrypto.h"

namespace vds {
  class security_walker {
  public:
    security_walker(
        const guid & user_id,
        const certificate & user_cert,
        const asymmetric_private_key & user_private_key);

    const guid & user_id() const {
      return this->user_id_;
    }

    const certificate & user_cert() const {
      return this->user_cert_;
    }

    const asymmetric_private_key & user_private_key() const {
      return this->user_private_key_;
    }

    void load(class database_transaction &t);

    void apply(
        const guid & channel_id,
        int message_id,
        const guid & read_cert_id,
        const guid & write_cert_id,
        const const_data_buffer & message,
        const const_data_buffer & signature);

    bool get_channel_write_certificate(const guid &channel_id,
                                       std::string & name,
                                       certificate &write_certificate,
                                     asymmetric_private_key &write_key);

  private:
    const guid user_id_;
    const certificate user_cert_;
    const asymmetric_private_key user_private_key_;

    struct channel_info {
      std::map<guid, certificate> read_certificates_;
      std::map<guid, certificate> write_certificates_;

      std::map<guid, asymmetric_private_key> read_private_keys_;
      std::map<guid, asymmetric_private_key> write_private_keys_;

      certificate read_certificate_;
      asymmetric_private_key read_private_key_;

      certificate write_certificate_;
      asymmetric_private_key write_private_key_;

      std::string name_;
    };

    std::map<guid, channel_info> channels_;

  };
}

#endif //__VDS_USER_MANAGER_SECURITY_WALKER_H_
