#ifndef __VDS_DATA_CHUNK_FILE_H_
#define __VDS_DATA_CHUNK_FILE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _chunk_file;
  class _chunk_file_creator;

  class chunk_file
  {
  public:



  private:
  };

  class chunk_file_creator
  {
  public:
    chunk_file_creator(
      size_t index,
      const std::string & meta_info
    );

    template <typename context_type>
    class handler : public sequence_step<context_type, void(void)>
    {
    public:

    };
  };
}

#endif//__VDS_DATA_CHUNK_FILE_H_
