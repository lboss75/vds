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
    vds::async_task<void> async_transaction(
        
        const std::function<void(class database_transaction & t)> & handler);

		vds::async_task<void> async_read_transaction(
				
				const std::function<void(class database_read_transaction & t)> & handler);

	void start(const service_provider * sp);
	void stop();
	vds::async_task<void> prepare_to_stop();

  size_t queue_length() const {
    return this->db_.queue_length();
  }

  private:
    database db_;

	static void migrate(class database_transaction & t, int64_t db_version);

  };

}


#endif //__VDS_DB_MODEL_DB_MODEL_H_
