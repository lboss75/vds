#ifndef __VDS_STORAGE_CHUNK_MANAGER_H_
#define __VDS_STORAGE_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _chunk_manager;
  
  class chunk_manager
  {
  public:
    chunk_manager(const service_provider & sp);
    ~chunk_manager();

    void add(const filename & fn, std::list<uint64_t> & parts);
    uint64_t add(const data_buffer & data);

  private:
    _chunk_manager * impl_;
  };
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
