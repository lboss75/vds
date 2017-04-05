#ifndef __VDS_CLIENT_CLIENT_H_
#define __VDS_CLIENT_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client_connection.h"
#include "client_logic.h"

namespace vds {

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
    certificate client_certificate_;
    asymmetric_private_key client_private_key_;
    std::unique_ptr<client_logic> logic_;
  };
  
  class iclient
  {
  public:
    iclient(const service_provider & sp, client * owner);
    
    async_task<> init_server(
      const std::string & user_login,
      const std::string & user_password);

    async_task<> upload_file(
      const std::string & login,
      const std::string & password,
      const std::string & name,
      const void * data,
      size_t data_size);

    async_task<data_buffer &&> download_data(
      const std::string & login,
      const std::string & password,
      const std::string & name);

  private:
    service_provider sp_;
    logger log_;
    client * owner_;


    async_task<
      const certificate & /*user_certificate*/,
      const asymmetric_private_key & /*user_private_key*/>
    authenticate(
      const std::string & login,
      const std::string & password);
  };
}


#endif // __VDS_CLIENT_CLIENT_H_
