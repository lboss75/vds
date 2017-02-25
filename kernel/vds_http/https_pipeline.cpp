/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "https_pipeline.h"
#include "https_pipeline_p.h"

vds::https_pipeline::https_pipeline(
  const vds::service_provider & sp,
  const std::string & address,
  int port,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
: impl_(new _https_pipeline(
  sp,
  this,
  address,
  port,
  client_certificate,
  client_private_key))
{
}

vds::https_pipeline::~https_pipeline()
{
}

void vds::https_pipeline::on_connected()
{
}

void vds::https_pipeline::on_error(std::exception* error)
{
}

void vds::https_pipeline::connect()
{
  this->impl_->connect();
}

void vds::https_pipeline::push(json_value* request)
{
  this->impl_->push(request);
}

const std::string & vds::https_pipeline::address() const
{
  return this->impl_->address();
}

int vds::https_pipeline::port() const
{
  return this->impl_->port();
}
//////////////////////////////////////////////////////////////////////////
vds::_https_pipeline::_https_pipeline(
  const service_provider & sp,
  https_pipeline * owner,
  const std::string & address,
  int port,
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
: sp_(sp),
owner_(owner),
log_(sp, "HTTPS pipeline"),
address_(address),
port_(port),
client_certificate_(client_certificate),
client_private_key_(client_private_key)
{
}

void vds::_https_pipeline::connect()
{
  auto sp = this->sp_.create_scope();
  sequence(
    socket_connect(sp),
    connection(sp, this)
  )
  (
   [this](){ this->owner_->on_connected(); },
   [this](std::exception * ex) { this->owner_->on_error(ex); },
   this->address_,
   this->port_
  );
}

void vds::_https_pipeline::push(json_value* request)
{
  json_writer writer;
  request->str(writer);
  
  std::function<void (const std::string & request)> request_callback;
  std::string request_data;
  
  {
    std::unique_lock<std::mutex> lock(this->request_mutex_);
    this->request_data_ += writer.str();
    
    if(this->request_callback_){
      request_callback.swap(this->request_callback_);
      request_data = this->request_data_;
      
      this->request_data_.clear();
    }
  }
  
  request_callback(request_data);
}

void vds::_https_pipeline::get_commands(const std::function<void (const std::string & request)> & callback)
{
  std::string request_data;
  {
    std::unique_lock<std::mutex> lock(this->request_mutex_);
    
    if(!this->request_data_.empty()){
      request_data = this->request_data_;
      this->request_data_.clear();
    }
    else {
      this->request_callback_ = callback;
      return;
    }
      
  }
  
  callback(request_data);
}


vds::_https_pipeline::connection::connection(
  const service_provider & sp,
  _https_pipeline * owner)
: sp_(sp),
owner_(owner)
{
}

