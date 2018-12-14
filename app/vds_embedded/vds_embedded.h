#ifndef __VDS_EMBEDDED_VDS_EMBEDDED_H_
#define __VDS_EMBEDDED_VDS_EMBEDDED_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <task_manager.h>
#include <mt_service.h>
#include "user_manager.h"
#include "vds_api.h"

namespace vds {

  class vds_embedded {
  public:
    vds_embedded();

    void set_root_folder(const std::string & root_folder);

    void server_root(const std::string & login, const std::string & password);

    const std::string & last_error() const {
      return this->last_error_;
    }

    void last_error(const std::string & value) {
      this->last_error_ = value;
    }

    void start(int port);
    void stop();
    bool local_storage_exists();

    class vds_session {
    public:
      vds_session(
        service_provider * sp,
        const std::shared_ptr<user_manager> & user_mng);

      const char * get_login_state() const;
      const char * get_device_storages();

      const char * prepare_device_storage();
      const char * add_device_storage(
        const std::string& name,
        const std::string& local_path,
        uint64_t reserved_size);

    private:
      service_provider * sp_;
      std::shared_ptr<user_manager> user_mng_;

      std::string last_result_;
    };

    vds_session * login(const std::string & login, const std::string & password);

  private:
    std::string root_folder_;
    std::string last_error_;

    service_registrator registrator_;
    task_manager task_manager_;
    mt_service mt_service_;
    file_logger logger_;
    network_service network_service_;
    crypto_service crypto_service_;
    server server_;

    service_provider * sp_;
  };
}

#endif //__VDS_EMBEDDED_VDS_EMBEDDED_H_
