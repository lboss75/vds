/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "server_http_api.h"
#include "server_http_api_p.h"


vds::async_task<> vds::server_http_api::start(
  const service_provider & sp,
  const std::string & address,
  int port,
  certificate & certificate,
  asymmetric_private_key & private_key)
{
  return static_cast<_server_http_api *>(this)->start(
    sp, address, port, certificate, private_key);
}

/////////////////////////////
static void collect_wwwroot(
  vds::http_router & router,
  const vds::foldername & folder,
  const std::string & root_folder
)
{
  if (!folder.exist()) {
    return;
  }

  folder.files(
    [&router, &root_folder](const vds::filename & fn) -> bool {
    router.add_file(
      root_folder + fn.name(),
      fn
    );

    return true;
  });

  folder.folders(
    [&router, &root_folder](const vds::foldername & fn) -> bool {
    collect_wwwroot(router, fn, root_folder + fn.name() + "/");
    return true;
  });
}

vds::_server_http_api::_server_http_api()
{
}

vds::async_task<> vds::_server_http_api::start(
  const service_provider & sp,
  const std::string & address,
  int port,
  certificate & certificate,
  asymmetric_private_key & private_key)
{
  this->router_.reset(new http_router());

  collect_wwwroot(
    *this->router_,
    foldername(foldername(persistence::current_user(sp), ".vds"), "wwwroot"),
    "/");

  this->router_->add_file(
    "/",
    filename(foldername(foldername(persistence::current_user(sp), ".vds"), "wwwroot"), "index.html"));

  //upnp_client upnp(sp);
  //upnp.open_port(8000, 8000, "TCP", "VDS Service");

  return create_async_task(
    [this, sp, address, port, &certificate, &private_key](
      const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp){
      sp.get<logger>().debug(sp, "Start HTTP sever %s:%d", address.c_str(), port);
      dataflow(
        socket_server(sp, address, port),
        vds::for_each<network_socket &>::create_handler(
          socket_session(*this->router_, certificate, private_key))
      )
      (done, on_error, sp);
    });
}

vds::_server_http_api::socket_session::socket_session(
  const http_router & router,
  const certificate & certificate,
  const asymmetric_private_key & private_key)
  : router_(router), certificate_(certificate),
  private_key_(private_key)
{
}

vds::_server_http_api::socket_session::handler::handler(
  const socket_session & owner,
  vds::network_socket & s)
: s_(std::move(s)),
  tunnel_(false, &owner.certificate_, &owner.private_key_),
  certificate_(owner.certificate_),
  private_key_(owner.private_key_),
  server_http_handler_(owner.router_),
  done_handler_(this),
  error_handler_([this](const service_provider & sp, std::exception_ptr) {delete this; }),
  http_server_done_([this](const service_provider & sp) {}),
  http_server_error_([this](const service_provider & sp, std::exception_ptr) {})
{
}

void vds::_server_http_api::socket_session::handler::start(const service_provider & sp)
{
  vds::dataflow(
    input_network_stream(sp, this->s_),
    ssl_input_stream(this->tunnel_),
    http_parser(sp),
    http_middleware<server_http_handler>(this->server_http_handler_),
    http_response_serializer(),
    ssl_output_stream(this->tunnel_),
    output_network_stream(sp, this->s_)
  )
  (
    this->done_handler_,
    this->error_handler_,
    sp);
}

vds::_server_http_api::server_http_handler::server_http_handler(
  const http_router & router)
: router_(router)
{  
}

