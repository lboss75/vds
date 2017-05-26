#include "stdafx.h"
#include "udp_socket.h"
#include "udp_socket_p.h"

vds::udp_datagram::udp_datagram()
{
}

void vds::udp_datagram::reset(const std::string & server, uint16_t port, const void * data, size_t data_size)
{
  this->impl_.reset(new _udp_datagram(server, port, data, data_size));
}

const std::string & vds::udp_datagram::server() const
{
  return this->impl_->server();
}

uint16_t vds::udp_datagram::port() const
{
  return this->impl_->port();
}

const void * vds::udp_datagram::data() const
{
  return this->impl_->data();
}

size_t vds::udp_datagram::data_size() const
{
  return this->impl_->data_size();
}

vds::udp_socket::udp_socket()
  : impl_(new _udp_socket())
{
}

vds::udp_socket::~udp_socket()
{
}

vds::async_stream<vds::udp_datagram> & vds::udp_socket::incoming()
{
  return this->impl_->incoming();
}

vds::async_stream<vds::udp_datagram> & vds::udp_socket::outgoing()
{
  return this->impl_->outgoing();
}

void vds::udp_socket::close()
{
  this->impl_->close();
}

vds::udp_server::udp_server()
{
}

vds::udp_server::~udp_server()
{
}

vds::udp_socket vds::udp_server::start(
  const service_provider & sp,
  const std::string & address,
  int port)
{
  this->impl_.reset(new _udp_server(address, port));
  return this->impl_->start(sp);

}

vds::udp_client::udp_client()
{
}

vds::udp_client::~udp_client()
{
}

vds::udp_socket vds::udp_client::start(const service_provider & sp)
{
  this->impl_.reset(new _udp_client());
  return this->impl_->start(sp);
}

void vds::udp_client::stop(const service_provider & sp)
{
}
