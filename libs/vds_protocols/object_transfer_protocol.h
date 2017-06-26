#ifndef __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_
#define __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _object_transfer_protocol;
  
  class object_transfer_protocol
  {
  public:
    object_transfer_protocol();
    ~object_transfer_protocol();
    
    void ask_object(
      const service_provider & sp,
      const guid & source_server_id,
      uint64_t object_index);
    
    _object_transfer_protocol * operator -> () const { return this->impl_; }

  private:
    _object_transfer_protocol * const impl_;
  };
  
}

#endif // __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_
