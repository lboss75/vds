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
        const std::shared_ptr<asymmetric_public_key> & public_key,
        const std::shared_ptr<asymmetric_private_key> & private_key,
        const std::string &name,
        uint64_t reserved_size,
        uint64_t free_size)
        : id_(id), public_key_(public_key),
          private_key_(private_key),
          name_(name),
          reserved_size_(reserved_size),
          free_size_(free_size) {}

    const std::string & id() const {
      return id_;
    }

    const std::shared_ptr<asymmetric_public_key> & public_key() const {
      return public_key_;
    }

    const std::shared_ptr<asymmetric_private_key> & private_key() const {
      return private_key_;
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
    std::shared_ptr<asymmetric_public_key> public_key_;
    std::shared_ptr<asymmetric_private_key> private_key_;

    std::string name_;
    uint64_t reserved_size_;
    uint64_t free_size_;
  };
}

#endif //__VDS_USER_MANAGER_DEVICE_INFO_H_
