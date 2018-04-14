#ifndef __VDS_USER_MANAGER_DEVICE_INFO_H_
#define __VDS_USER_MANAGER_DEVICE_INFO_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "asymmetriccrypto.h"

namespace vds {
  class device_info {
  public:
    device_info(
        const std::string &id,
        const certificate &cert,
        const asymmetric_private_key &cert_key,
        const std::string &name,
        uint64_t reserved_size,
        uint64_t free_size)
        : id_(id), cert_(cert),
          cert_key_(cert_key),
          name_(name),
          reserved_size_(reserved_size),
          free_size_(free_size) {}

    const std::string & id() const {
      return id_;
    }

    const certificate & cert() const {
      return cert_;
    }

    const asymmetric_private_key & cert_key() const {
      return cert_key_;
    }

    const std::string & name() const {
      return name_;
    }

    uint64_t reserved_size() const {
      return reserved_size_;
    }

    uint64_t free_size() const {
      return free_size_;
    }

  private:
    std::string id_;
    certificate cert_;
    asymmetric_private_key cert_key_;

    std::string name_;
    uint64_t reserved_size_;
    uint64_t free_size_;
  };
}

#endif //__VDS_USER_MANAGER_DEVICE_INFO_H_
