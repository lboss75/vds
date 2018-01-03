#ifndef __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
#define __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include <string>
#include "const_data_buffer.h"

namespace vds {
  class chunk_manager {
  public:
		static std::string pack_block(
        class database_transaction & t,
        const class const_data_buffer & data);

	static const_data_buffer get_block(
		class database_transaction & t,
		const std::string & block_id);

  private:
    std::shared_ptr<class _chunk_manager> impl_;
  };
}

#endif //__VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
