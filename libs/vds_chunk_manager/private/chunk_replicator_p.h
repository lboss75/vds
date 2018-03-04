#ifndef __VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_P_H_
#define __VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "chunk.h"

namespace vds {

  class _chunk_replicator : public std::enable_shared_from_this<_chunk_replicator> {
  public:
    _chunk_replicator();

    void start(const service_provider & sp);
    void stop(const service_provider & sp);
	  void apply(const service_provider& sp, const guid& partner_id, const p2p_messages::chunk_send_replica& message);

    void put_object(
        const service_provider &sp,
        class database_transaction &t,
        const const_data_buffer &object_id,
        const const_data_buffer & data);

  private:
    timer update_timer_;

    std::mutex update_timer_mutex_;
    bool in_update_timer_;

    std::mutex generators_mutex_;
    std::unordered_map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;

    void update_replicas(
        const service_provider & sp,
        class database_transaction & t);

    const_data_buffer generate_replica(uint16_t replica, const void * data, size_t size);
		void store_replica(
				const service_provider& sp,
				class database_transaction& t,
				const p2p_messages::chunk_send_replica& message);
  };
}


#endif //__VDS_CHUNK_MANAGER_CHUNK_REPLICATOR_P_H_
