#ifndef __VDS_CLIENT_CLIENT_H_
#define __VDS_CLIENT_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client_connection.h"
#include "client_logic.h"

namespace vds {
  class _client;
  class client : public iservice
  {
  public:
    client();
    ~client();

    // Inherited via iservice
    void register_services(service_registrator &) override;
    void start(const service_provider & sp) override;
    void stop(const service_provider & sp) override;


    void connection_closed();
    void connection_error();

  private:
    friend class iclient;
    friend class _client;

    certificate client_certificate_;
    asymmetric_private_key client_private_key_;
    std::unique_ptr<client_logic> logic_;

    std::unique_ptr<_client> impl_;
  };
  
  class iclient
  {
  public:
    iclient(client * owner);
    
    async_task<
      const vds::certificate & /*server_certificate*/,
      const vds::asymmetric_private_key & /*private_key*/>
      init_server(
      const std::string & user_login,
      const std::string & user_password);

    async_task<const std::string& /*version_id*/> upload_file(
      const std::string & login,
      const std::string & password,
      const std::string & name,
      const void * data,
      size_t data_size);

    async_task<const_data_buffer &&> download_data(
      const std::string & login,
      const std::string & password,
      const std::string & name);

  private:
    client * owner_;
  };
}


#endif // __VDS_CLIENT_CLIENT_H_
