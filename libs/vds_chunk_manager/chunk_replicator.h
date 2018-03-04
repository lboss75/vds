#ifndef __VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_H_
#define __VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <messages/chunk_offer_replica.h>
#include <messages/chunk_have_replica.h>
#include <stdafx.h>
#include "service_provider.h"
#include "messages/chunk_send_replica.h"
#include "messages/chunk_query_replica.h"

namespace vds {

  class chunk_replicator {
  public:
    static const uint16_t MIN_HORCRUX = 512;
    static const uint16_t GENERATE_HORCRUX = 1024;

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

    void apply(const service_provider& sp, const guid& partner_id, const p2p_messages::chunk_send_replica& message);
    void apply(const service_provider& sp, const guid& partner_id, const p2p_messages::chunk_query_replica & message);
    void apply(const service_provider& sp, const guid& partner_id, const p2p_messages::chunk_offer_replica & message);
    void apply(const service_provider& sp, const guid& partner_id, const p2p_messages::chunk_have_replica & message);

    operator bool () const {
      return nullptr != this->impl_.get();
    }

    void put_object(
        const service_provider &sp,
        class database_transaction &t,
        const const_data_buffer &object_id,
        const const_data_buffer & data);

    const_data_buffer get_object(
        const service_provider &sp,
        class database_transaction &t,
        const const_data_buffer &object_id);

  private:
    std::shared_ptr<class _chunk_replicator> impl_;
  };
}


#endif //__VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_H_
