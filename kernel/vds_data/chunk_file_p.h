#ifndef __VDS_DATA_CHUNK_FILE_P_H_
#define __VDS_DATA_CHUNK_FILE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_file_creator.h"

namespace vds {
  class _chunk_file_creator
  {
  public:
    _chunk_file_creator(
      certificate & cert,
      asymmetric_private_key & key,
      size_t index,
      const std::string & meta_info
    );

    void update(const void * data, size_t len);
    void final();

  private:
    file target_;
  };

}

#endif//__VDS_DATA_CHUNK_FILE_P_H_
