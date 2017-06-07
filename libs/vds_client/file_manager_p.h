#ifndef __VDS_CLIENT_FILE_MANAGER_P_H_
#define __VDS_CLIENT_FILE_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class _file_manager
  {
  public:
      _file_manager();
      ~_file_manager();

    async_task<> upload_file(
      const service_provider & sp,
      const std::string & name,
      const filename & fn);

  private:
  };
  
}

#endif // __VDS_CLIENT_FILE_MANAGER_P_H_
