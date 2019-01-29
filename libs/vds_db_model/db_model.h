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
        const std::function<expected<void>(class database_transaction & t)> & handler);

		vds::async_task<vds::expected<void>> async_read_transaction(
				const std::function<expected<void>(class database_read_transaction & t)> & handler);

    async_task<vds::expected<void>> async_transaction(
      const char * file_name,
      int line,
      const std::function<expected<void>(class database_transaction & t)> & handler);

    vds::async_task<vds::expected<void>> async_read_transaction(
      const char * file_name,
      int line,
      const std::function<expected<void>(class database_read_transaction & t)> & handler);

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
