/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client.h"
#include "client_connection.h"

vds::client::client()
{
}

vds::client::~client()
{
}

void vds::client::register_services(service_registrator &)
{
}

void vds::client::start(const service_provider & sp)
{
  //Load certificates
  this->client_certificate_.load(filename(foldername(persistence::current_user(), ".vds"), "cacert.pem"));
  this->client_private_key_.load(filename(foldername(persistence::current_user(), ".vds"), "cakey.pem"));

  this->logic_.reset(new client_logic(sp, this->client_certificate_, this->client_private_key_));
  this->logic_->start();
}

void vds::client::stop(const service_provider & sp)
{
  this->logic_->stop();
}

void vds::client::connection_closed()
{
}

void vds::client::connection_error()
{
}

void vds::client::node_install(const std::string & login, const std::string & password)
{
  //messages::ask_certificate_and_key(login)
}
