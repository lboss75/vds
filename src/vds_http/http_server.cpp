/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_server.h"
#include "http_parser.h"
#include "pipeline.h"
#include "http_middleware.h"
#include "http_response_stream.h"

#include <iostream>

vds::http_server::http_server(
  http_router * router)
  : address_("127.0.0.1"), port_(8080), router_(router)
{
}

void vds::http_server::register_services(vds::service_registrator&)
{

}

void vds::http_server::start(
  const vds::service_provider & sp)
{
  sp.get<inetwork_manager>()
    .start_server(sp, this->address_, this->port_,
      [this, sp](const network_socket & s) {
        std::shared_ptr<std::vector<char>> buffer(new std::vector<char>(1024));
        
        pipeline(
          [s, sp, buffer](
            const std::function<void(const char *, size_t)> & done,
            const error_handler_t & on_error){
            s.read_async(
              [buffer, done](size_t readed){
                std::cout
                  << "readed " << readed << "bytes:\n"
                  << std::string(buffer->data(), readed) << "\n";
                done(buffer->data(), readed);
              },
              on_error,
              sp,
              buffer->data(),
              buffer->size());
          },
          http_parser(),
          http_middleware(this->router_),
          http_response_stream(),       
          [this, s, sp](
            const std::function<void(void)> & done,
            const vds::error_handler_t& on_error,
            const char * data,
            size_t len
          ){
            std::cout
              << "send " << len << "bytes:\n"
              << std::string(data, len) << "\n";
            s.write_async(
              done,
              on_error,
              sp,
              data,
              len
            );
          }
        )(
          []() {
            //TODO: s.close();
          },
          [](std::exception * ex){
            std::cerr << ex->what() << "\n";
            delete ex;
          }
        );
      },
      [](std::exception * ex) {
        delete ex;
      });
}

void vds::http_server::stop(const vds::service_provider&)
{
}
