#ifndef __VDS_STORAGE_ENDPOINT_H_
#define __VDS_STORAGE_ENDPOINT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class istorage;

  class endpoint
  {
  public:
    endpoint(
      uint64_t id,
      const std::string & addresses
    ) : id_(id), addresses_(addresses)
    {
    }
    
    uint64_t id() const { return this->id_; }

    const std::string & addresses() const {
      return this->addresses_;
    }

  private:
    uint64_t id_;
    std::string addresses_;
  };
}

#endif // __VDS_STORAGE_ENDPOINT_H_
