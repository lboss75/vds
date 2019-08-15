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
    vds::async_task<vds::expected<void>> async_transaction(
        lambda_holder_t<expected<void>, class database_transaction &> handler);

		vds::async_task<vds::expected<void>> async_read_transaction(
      lambda_holder_t<expected<void>, class database_read_transaction &> handler);

    expected<void> start(const service_provider * sp);
    expected<void> stop();
	vds::async_task<vds::expected<void>> prepare_to_stop();

  size_t queue_length() const {
    return this->db_.queue_length();
  }

  private:
    database db_;
    const service_provider * sp_;

	static expected<void> migrate(class database_transaction & t, int64_t db_version);

  };

}

#endif //__VDS_DB_MODEL_DB_MODEL_H_
