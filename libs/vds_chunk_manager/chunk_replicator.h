#ifndef __VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_H_
#define __VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "service_provider.h"

namespace vds {

  class chunk_replicator {
  public:
    static const uint16_t MIN_HORCRUX = 512;
    static const uint16_t GENERATE_HORCRUX = 1024;

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

  private:
    std::shared_ptr<class _chunk_replicator> impl_;
  };
}


#endif //__VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_H_
