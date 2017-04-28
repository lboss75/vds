#ifndef __VDS_PROTOCOLS_FILE_MANAGER_H_
#define __VDS_PROTOCOLS_FILE_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _file_manager;
  class server;

  class file_manager
  {
  public:
    async_task<> put_file(
      const service_provider & sp,
      const std::string & version_id,
      const std::string & user_login,
      const std::string & name,
      const filename & fn);

    async_task<const filename&> download_file(
      const service_provider & sp,
      const std::string & user_login,
      const std::string & name);

    async_task<const filename&> download_file(
      const service_provider & sp,
      const guid & server_id,
      const std::string & version_id);


  private:
    friend class server;
    file_manager(_file_manager * impl);

    _file_manager * const impl_;
  };
}

#endif // __VDS_PROTOCOLS_FILE_MANAGER_H_
