#ifndef __VDS_PROTOCOLS_FILE_MANAGER_P_H_
#define __VDS_PROTOCOLS_FILE_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class iconnection_manager;

  class _file_manager : public file_manager
  {
  public:
    _file_manager();

    async_task<> put_file(
      const service_provider & sp,
      const std::string & version_id,
      const std::string & user_login,
      const std::string & name,
      const const_data_buffer & meta_info,
      const filename & fn);

    async_task<const_data_buffer /*meta_info*/, filename> download_file(
      const service_provider & sp,
      const std::string & user_login,
      const std::string & name);

    async_task<const_data_buffer /*meta_info*/, filename> download_file(
      const service_provider & sp,
      const guid & server_id,
      const std::string & version_id);
  };
}

#endif // __VDS_PROTOCOLS_FILE_MANAGER_P_H_
