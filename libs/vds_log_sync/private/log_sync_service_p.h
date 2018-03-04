#ifndef __VDS_LOG_SYNC_LOG_SYNC_SERVICE_P_H_
#define __VDS_LOG_SYNC_LOG_SYNC_SERVICE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include "task_manager.h"
#include "p2p_node_info.h"

namespace vds {

  class _log_sync_service : public std::enable_shared_from_this<_log_sync_service> {
  public:
    _log_sync_service();
    ~_log_sync_service();

    void start(const service_provider & sp);
    async_task<> prepare_to_stop(const service_provider &sp);
    void stop(const service_provider & sp);

    void get_statistic(class database_transaction & t, sync_statistic & result);

    void apply(
        const vds::service_provider &sp,
        const vds::guid &partner_id,
        const vds::p2p_messages::channel_log_state &message);

	void apply(
		const service_provider &sp,
		const guid &partner_id,
		const p2p_messages::channel_log_request & message);

	void apply(
		const vds::service_provider &sp,
		const vds::guid &partner_id,
		const vds::p2p_messages::channel_log_record &message);
    void add_subscriber(
      const service_provider& sp,
      const guid & channel_id,
      const guid & source_node_id);
    void send_to_subscribles(
      const service_provider& sp,
      database_transaction& t,
      const guid& channel_id);

  private:
    timer update_timer_;

    std::mutex state_mutex_;
    bool sycn_scheduled_;

    std::shared_mutex channel_subscribers_mutex_;
    std::map<guid, std::set<guid>> channel_subscribers_;

    void sync_process(const service_provider & sp, class database_transaction & t);

		void send_current_state(
				const service_provider &sp,
				database_transaction &t,
				class p2p_network *p2p);

		void ask_unknown_records(
				const vds::service_provider &sp,
				database_transaction &t,
				class p2p_network *p2p) const;
    void ask_unknown_certificates(
      const service_provider& sp,
      database_transaction& t,
      class p2p_network * p2p);

    void try_to_validate_records(const service_provider &sp, database_transaction &t);
  };

}


#endif //__VDS_LOG_SYNC_LOG_SYNC_SERVICE_P_H_
