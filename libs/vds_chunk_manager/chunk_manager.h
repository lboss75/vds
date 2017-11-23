#ifndef __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
#define __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include <string>

namespace vds {
  class chunk_manager {
  public:
    typedef std::string block_id_t;
    typedef class symmetric_key block_key_t;

    static block_id_t pack_block(
        class database_transaction & t,
        const class const_data_buffer & data);

	static const_data_buffer get_block(
		class database_transaction & t,
		const block_id_t & block_id);

  private:
    std::shared_ptr<class _chunk_manager> impl_;

  };
}

#endif //__VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
