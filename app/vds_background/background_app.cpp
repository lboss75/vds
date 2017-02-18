/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "background_app.h"
#include "http_router.h"

static void collect_wwwroot(
  vds::http_router & router,
  const vds::foldername & folder,
  const std::string & root_folder
)
{
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

vds::background_app::background_app()
  :
  start_server_command_set_("Server start", "Start web server", "start", "server"),
  http_server_done_([this]() { this->http_server_closed(); }),
  http_server_error_([this](std::exception * ex) { this->http_server_error(ex); })
{
}

void vds::background_app::main(const service_provider & sp)
{
  if(this->current_command_set_ == &this->start_server_command_set_){
    std::cout << "Waiting for network\n";
    
    sp.get<vsr_protocol::iclient>().subscribe_client_id_assigned([this](vsr_protocol::client & client){
      this->cliend_id_assigned_.set();
    });
    
    if(!this->cliend_id_assigned_.wait_for(std::chrono::seconds(5))){
      sp.get<vsr_protocol::iserver>().start_standalone();
    }

    this->router_.reset(new http_router(sp));

    collect_wwwroot(
      *this->router_,
      foldername(foldername(persistence::current_user(), ".vds"), "wwwroot"),
      "/");

    this->router_->add_file(
      "/",
      filename(foldername(foldername(persistence::current_user(), ".vds"), "wwwroot"), "index.html"));

      //upnp_client upnp(sp);
      //upnp.open_port(8000, 8000, "TCP", "VDS Service");
      
      this->certificate_.load(filename(foldername(persistence::current_user(), ".vds"), "cacert.crt"));
      this->private_key_.load(filename(foldername(persistence::current_user(), ".vds"), "cakey.pem"));

      sequence(
        socket_server(sp, "127.0.0.1", 8050),
        vds::for_each<network_socket>::create_handler(
          socket_session(sp, *this->router_, this->certificate_, this->private_key_))
      )
      (
        this->http_server_done_,
        this->http_server_error_
      );

    for (;;) {
      std::cout << "Enter command:\n";

      std::string cmd;
      std::cin >> cmd;

      if ("exit" == cmd) {
        break;
      }
    }
  }
}

void vds::background_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->network_service_);
  registrator.add(this->crypto_service_);
  registrator.add(this->task_manager_);
  
  if (&this->start_server_command_set_ == this->current_command_set_) {
    registrator.add(this->storage_);
    registrator.add(this->client_);
    registrator.add(this->server_);
  }
}

void vds::background_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->start_server_command_set_);
}

void vds::background_app::http_server_closed()
{
  std::cout << "HTTP server closed\n";
}

void vds::background_app::http_server_error(std::exception * ex)
{
  std::cout << "Server error: " << ex->what() << "\n";
  delete ex;
}

vds::background_app::socket_session::socket_session(
  const service_provider & sp,
  const http_router & router,
  const certificate & certificate,
  const asymmetric_private_key & private_key)
  : sp_(sp), router_(router), certificate_(certificate),
  private_key_(private_key)
{
}

vds::background_app::socket_session::handler::handler(
  const socket_session & owner,
  vds::network_socket & s)
: sp_(owner.sp_),
  s_(std::move(s)),
  tunnel_(false, &owner.certificate_, &owner.private_key_),
  certificate_(owner.certificate_),
  private_key_(owner.private_key_),
  server_logic_(owner.sp_, tunnel_, owner.router_),
  done_handler_(this),
  error_handler_([this](std::exception *) {delete this; }),
  http_server_done_([this]() {}),
  http_server_error_([this](std::exception *) {})
{
}

void vds::background_app::socket_session::handler::start()
{
  vds::sequence(
    input_network_stream(this->sp_, this->s_),
    ssl_input_stream(this->tunnel_),
    http_parser(this->sp_),
    http_middleware<server_logic>(this->server_logic_),
    http_response_serializer(),
    ssl_output_stream(this->tunnel_),
    output_network_stream(this->sp_, this->s_)
  )
  (
    this->done_handler_,
    this->error_handler_
  );
}
