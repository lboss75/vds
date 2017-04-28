/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server.h"
#include "server_http_api.h"
#include "server_http_api_p.h"

vds::server_http_api::server_http_api(const service_provider& sp)
: sp_(sp)
{
}

vds::async_task<> vds::server_http_api::start(
  const std::string & address,
  int port,
  certificate & certificate,
  asymmetric_private_key & private_key)
{
  this->impl_.reset(new _server_http_api(this->sp_));
  return this->impl_->start(address, port, certificate, private_key);
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

vds::_server_http_api::_server_http_api(const service_provider& sp)
: sp_(sp), log_(sp, "HTTP API")
{
}

vds::async_task<> vds::_server_http_api::start(
  const std::string & address,
  int port,
  certificate & certificate,
  asymmetric_private_key & private_key)
{
  this->router_.reset(new http_router(this->sp_));

  collect_wwwroot(
    *this->router_,
    foldername(foldername(persistence::current_user(this->sp_), ".vds"), "wwwroot"),
    "/");

  this->router_->add_file(
    "/",
    filename(foldername(foldername(persistence::current_user(this->sp_), ".vds"), "wwwroot"), "index.html"));

  //upnp_client upnp(sp);
  //upnp.open_port(8000, 8000, "TCP", "VDS Service");

  return create_async_task(
    [this, address, port, &certificate, &private_key](
      const std::function<void(void)> & done,
      const error_handler & on_error){
      sp.get<logger>().debug(sp, "Start HTTP sever %s:%d", address.c_str(), port);
      dataflow(
        socket_server(this->sp_, address, port),
        vds::for_each<const service_provider &, network_socket &>::create_handler(
          socket_session(*this->router_, certificate, private_key))
      )
      (done, on_error);
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
  const service_provider & sp,
  vds::network_socket & s)
  : sp_(sp),
  s_(std::move(s)),
  tunnel_(sp, false, &owner.certificate_, &owner.private_key_),
  certificate_(owner.certificate_),
  private_key_(owner.private_key_),
  server_http_handler_(sp, owner.router_),
  done_handler_(this),
  error_handler_([this](std::exception_ptr) {delete this; }),
  http_server_done_([this]() {}),
  http_server_error_([this](std::exception_ptr) {})
{
}

void vds::_server_http_api::socket_session::handler::start()
{
  vds::dataflow(
    input_network_stream(this->sp_, this->s_),
    ssl_input_stream(this->tunnel_),
    http_parser(this->sp_),
    http_middleware<server_http_handler>(this->server_http_handler_),
    http_response_serializer(),
    ssl_output_stream(this->tunnel_),
    output_network_stream(this->sp_, this->s_)
  )
  (
    this->done_handler_,
    this->error_handler_
    );
}

vds::_server_http_api::server_http_handler::server_http_handler(
  const service_provider & sp,
  const http_router & router)
: server_json_api_(sp),
  server_json_client_api_(sp),
  router_(router)
{  
}

