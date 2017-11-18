#ifndef __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
#define __VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class chunk_manager {
  public:
    typedef const_data_buffer block_id_t;
    typedef symmetric_key block_key_t;

    static block_id_t pack_block(
        class database_transaction & t,
        const const_data_buffer & data);

  private:
    std::shared_ptr<class _chunk_manager> impl_;

  };
}

#endif //__VDS_CHUNK_MANAGER_CHUNK_MANAGER_H_
