#ifndef __VDS_LOG_SYNC_LOG_SYNC_SERVICE_P_H_
#define __VDS_LOG_SYNC_LOG_SYNC_SERVICE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "task_manager.h"

namespace vds {

  class _log_sync_service : public std::enable_shared_from_this<_log_sync_service> {
  public:
    _log_sync_service();
    ~_log_sync_service();

    void start(const service_provider & sp);
    async_task<> prepare_to_stop(const service_provider &sp);
    void stop(const service_provider & sp);

    void get_statistic(sync_statistic & result);

  private:
    timer update_timer_;

    std::mutex state_mutex_;
    bool sycn_scheduled_;

    void sync_process(const service_provider & sp, database_transaction & t);
    void request_unknown_records(
        const service_provider &sp,
        class p2p_network *p2p,
        const std::list<std::string> &record_ids);

  };

}


#endif //__VDS_LOG_SYNC_LOG_SYNC_SERVICE_P_H_
