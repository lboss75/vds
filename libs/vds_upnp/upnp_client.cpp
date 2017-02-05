/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "upnp_client.h"

vds::upnp_client::upnp_client(const vds::service_provider& sp)
: log_(sp, "UPNP client"),
  devlist_(nullptr)
{
  memset(&this->upnp_urls_, 0, sizeof(this->upnp_urls_));
}

vds::upnp_client::~upnp_client()
{
  FreeUPNPUrls(&this->upnp_urls_);
  
  if(nullptr != this->devlist_){
    freeUPNPDevlist(this->devlist_);
  }
}


bool vds::upnp_client::open_port(
  uint16_t internal_port,
  uint16_t external_port,
  const std::string & protocol,
  const std::string & description
)
{
  this->log_(debug("Execute the UPNP discovery process"));
  
  int error_code;
  this->devlist_ = upnpDiscover(2000, NULL, NULL, 0, 0, &error_code);
  if(nullptr == this->devlist_) {
    this->log_(error("No UPnP device found on the network. Error:") << error_code);
  }
  else {
    auto status = UPNP_GetValidIGD(
      this->devlist_,
      &this->upnp_urls_,
      &this->igd_data_,
      this->lanaddr_,
      sizeof(this->lanaddr_));
    if(0 == status){
      this->log_(error("No IGD found"));
    }
    else {
      this->log_(debug("Found IGD ") << this->upnp_urls_.controlURL << " (status " << status << ")");
      
      int r = UPNP_AddPortMapping(
				this->upnp_urls_.controlURL,
				this->igd_data_.first.servicetype,
				std::to_string(internal_port).c_str(),
				std::to_string(external_port).c_str(),
				this->lanaddr_,
				description.c_str(),
				protocol.c_str(),
				0,
				0);
      if (r == UPNPCOMMAND_SUCCESS) {
        this->log_(debug("Added mapping ") << protocol << " " << external_port << " to " << this->lanaddr_ << internal_port);
        return true;
      }
      else {
        this->log_(error("open_port failed with code ") << r << "(" << strupnperror(r) << ")");
      }
    }
  }
  
  return false;
}

void vds::upnp_client::close_port(
  uint16_t external_port,
  const std::string& protocol)
{
  int r = UPNP_DeletePortMapping(
    this->upnp_urls_.controlURL,
    this->igd_data_.first.servicetype,
    std::to_string(external_port).c_str(),
    protocol.c_str(),
    0);
  if (r == UPNPCOMMAND_SUCCESS) {
    this->log_(debug("Removed mapping ") << protocol << " " << external_port);
  }
  else {
    this->log_(error("close_port failed with code ") << r << "(" << strupnperror(r) << ")");
  }
}
