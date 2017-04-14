/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "server_p.h"
#include "consensus_protocol.h"
#include "node_manager.h"
#include "user_manager.h"
#include "cert_manager.h"
#include "server_http_api.h"
#include "server_http_api_p.h"
#include "server_connection.h"
#include "server_udp_api.h"
#include "node_manager.h"
#include "node_manager_p.h"

vds::server::server(bool for_init)
: for_init_(for_init), impl_(new _server(this))
{
}

vds::server::~server()
{
  delete impl_;
}



void vds::server::register_services(service_registrator& registrator)
{
  registrator.add_factory<iserver>([this](const service_provider &, bool &)->iserver{
    return iserver(this);
  });
  
  registrator.add_factory<istorage_log>([this](const service_provider &, bool &)->istorage_log{
    return istorage_log(this->impl_->storage_log_.get());
  });
  
  registrator.add_factory<ichunk_manager>([this](const service_provider &, bool &)->ichunk_manager{
    return ichunk_manager(this->impl_->chunk_manager_.get());
  });
  
  registrator.add_factory<iserver_database>([this](const service_provider &, bool &)->iserver_database{
    return iserver_database(this->impl_->server_database_.get());
  });
  
  registrator.add_factory<ilocal_cache>([this](const service_provider &, bool &)->ilocal_cache{
    return ilocal_cache(this->impl_->local_cache_.get());
  });

  registrator.add_factory<node_manager>([this](const service_provider &, bool &)->node_manager {
    return node_manager(this->impl_->node_manager_.get());
  });

  registrator.add_factory<cert_manager>([this](const service_provider &, bool &)->cert_manager {
    return cert_manager();
  });
}

void vds::server::start(const service_provider& sp)
{
  if(this->for_init_){
    this->for_init_ = false;
    return;
  }
  
  this->impl_->start(sp);
}

void vds::server::stop(const service_provider& sp)
{
  this->impl_->stop(sp);
}

void vds::server::set_port(size_t port)
{
  this->impl_->set_port(port);
}


vds::iserver::iserver(vds::server* owner)
: owner_(owner)
{
}
/////////////////////////////////////////////////////////////////////////////////////////////

vds::_server::_server(server * owner)
  : owner_(owner)
{
}

vds::_server::~_server()
{
}

void vds::_server::start(const service_provider& sp)
{
  this->certificate_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.crt"));
  this->private_key_.load(filename(foldername(persistence::current_user(sp), ".vds"), "server.pkey"));

  this->storage_log_.reset(new storage_log(
    sp,
    server_certificate::server_id(this->certificate_),
    this->certificate_,
    this->private_key_));
  this->server_database_.reset(new server_database(sp));

  this->connection_manager_.reset(new connection_manager(sp, this->certificate_.subject()));
  this->consensus_server_protocol_.reset(new consensus_protocol::server(sp, this->certificate_, this->private_key_, *this->connection_manager_));
  this->node_manager_.reset(new _node_manager(sp));
  this->server_http_api_.reset(new server_http_api(sp));
  this->server_udp_api_.reset(new server_udp_api(sp));
  this->server_connection_.reset(new server_connection(sp, this->server_udp_api_.get()));
  this->peer_network_.reset(new peer_network(sp));
  this->local_cache_.reset(new local_cache(sp));
  this->chunk_manager_.reset(new chunk_manager(sp));

  this->server_database_->start();
  this->storage_log_->start();
  this->local_cache_->start();
  this->chunk_manager_->start();

  this->consensus_server_protocol_->start();

  this->server_connection_->start();
  this->server_http_api_->start("127.0.0.1", this->port_, this->certificate_, this->private_key_)
    .wait(
      [log = logger(sp, "HTTP Server API")](){log.trace("Server closed"); },
      [log = logger(sp, "HTTP Server API")](std::exception_ptr ex){log.trace("Server error %s", exception_what(ex).c_str()); });

  this->server_udp_api_->start("127.0.0.1", this->port_);

  //this->peer_network_->start();


  //this->client_logic_.reset(new client_logic(sp, &this->certificate_, &this->private_key_, sp.get<istorage>().get_storage_log().get_endpoints()));
  //this->client_logic_->start();
}

void vds::_server::stop(const service_provider& sp)
{

}

void vds::_server::set_port(size_t port)
{
  this->port_ = port;
}
