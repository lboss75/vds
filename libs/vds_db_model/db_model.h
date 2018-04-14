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

		async_task<> async_read_transaction(
				const service_provider & sp,
				const std::function<void(class database_transaction & t)> & handler);

	void start(const service_provider & sp);
	void stop(const service_provider & sp);
	async_task<> prepare_to_stop(const vds::service_provider &sp);

  private:
    database db_;

	static void migrate(class database_transaction & t, uint64_t db_version);

  };

}


#endif //__VDS_DB_MODEL_DB_MODEL_H_
