#ifndef __VDS_EMBEDDED_VDS_EMBEDDED_H_
#define __VDS_EMBEDDED_VDS_EMBEDDED_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <task_manager.h>
#include <mt_service.h>

namespace vds {

  class vds_embedded {
  public:

    void set_root_folder(const std::string & root_folder);

    void server_root(const std::string & login, const std::string & password);

    const std::string & last_error() const {
      return this->last_error_;
    }

    void start();

    class vds_session {
    public:

    private:

    };

    vds_session * login(const std::string & login, const std::string & password);

  private:
    std::string root_folder_;
    std::string last_error_;

    task_manager task_manager_;
    mt_service mt_service_;
    network_service network_service_;
    crypto_service crypto_service_;
    server server_;
  };
}

#endif //__VDS_EMBEDDED_VDS_EMBEDDED_H_
