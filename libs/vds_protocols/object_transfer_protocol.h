#ifndef __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_
#define __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_task_manager.h"

namespace vds {
  class _object_transfer_protocol;
  
  class object_transfer_protocol
  {
  public:
    object_transfer_protocol();
    ~object_transfer_protocol();
      
    void chunk_require(
      const service_provider & sp,
      database_transaction & tr,
      const guid & source_server_id,
      uint64_t chunk_index);
    
    _object_transfer_protocol * operator -> () const { return this->impl_; }

  private:
    _object_transfer_protocol * const impl_;
  };
  
}

#endif // __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_H_
