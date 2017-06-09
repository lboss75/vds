#ifndef __VDS_CLIENT_FILE_MANAGER_P_H_
#define __VDS_CLIENT_FILE_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  class asymmetric_private_key;
  class certificate;
  class filename;
  class foldername;
  class guid;
  class symmetric_key;
  
  class _file_manager
  {
  public:
    static async_task<size_t /*body_size*/, size_t /*tail_size*/> crypt_file(
      const service_provider & sp,
      size_t length,
      const symmetric_key & transaction_key,
      const filename & fn,
      const filename & tmp_file);
  };
  
}

#endif // __VDS_CLIENT_FILE_MANAGER_P_H_
