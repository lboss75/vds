#ifndef __VDS_DB_MODEL_DB_MODEL_H_
#define __VDS_DB_MODEL_DB_MODEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database.h"

namespace vds {

  class db_model {
  public:
    async_task<> async_transaction(
        const service_provider & sp,
        const std::function<void(class database_transaction & t)> & handler);

  private:
    database db_;

  };

}


#endif //__VDS_DB_MODEL_DB_MODEL_H_
