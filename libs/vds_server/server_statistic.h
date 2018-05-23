#ifndef __VDS_SERVER_SERVER_STATISTIC_H_
#define __VDS_SERVER_SERVER_STATISTIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "route_statistic.h"
#include "sync_statistic.h"
#include "session_statistic.h"

namespace vds {

  struct server_statistic {
    sync_statistic sync_statistic_;
    route_statistic route_statistic_;
    session_statistic session_statistic_;
  };

}
#endif //__VDS_SERVER_SERVER_STATISTIC_H_
