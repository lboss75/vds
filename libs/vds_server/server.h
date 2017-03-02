#ifndef __VDS_SERVER_SERVER_H_
#define __VDS_SERVER_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_logic.h"

namespace vds {
  class cert_manager;
  class node_manager;
  class user_manager;
  class server_http_api;
  class storage_log;

  class server : public iservice
  {
  public:
    server();
    ~server();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
    
  private:
    friend class iserver;

    certificate certificate_;
    asymmetric_private_key private_key_;

    std::unique_ptr<consensus_protocol::server> consensus_server_protocol_;
    std::unique_ptr<cert_manager> cert_manager_;
    std::unique_ptr<node_manager> node_manager_;
    std::unique_ptr<user_manager> user_manager_;
    
    std::unique_ptr<server_http_api> server_http_api_;
  };
  
  class iserver
  {
  public:
    iserver(server * owner);
    
    consensus_protocol::server * consensus_server_protocol() const { return this->owner_->consensus_server_protocol_.get(); }
    cert_manager * get_cert_manager() const { return this->owner_->cert_manager_.get(); }
    node_manager * get_node_manager() const { return this->owner_->node_manager_.get(); }
    user_manager * get_user_manager() const { return this->owner_->user_manager_.get(); }

    void start_http_server(const std::string & address, int port);

  private:
    server * owner_;
  };
}

#endif // __VDS_SERVER_SERVER_H_
