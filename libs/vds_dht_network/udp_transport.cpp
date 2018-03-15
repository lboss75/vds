//
// Created by vadim on 15.03.18.
//

#include "udp_transport.h"
#include "dht_message_type.h"
#include "dht_session.h"

void vds::dht::network::udp_transport::start(
    const vds::service_provider &sp,
    const vds::udp_socket &s) {
  this->continue_read(sp, s);
}

void vds::dht::network::udp_transport::continue_read(
    const vds::service_provider &sp,
    const vds::udp_socket &s) {

  s.read_async().execute([sp, s, pthis = this->shared_from_this()](
      const std::shared_ptr<std::exception> & ex,
      const vds::udp_datagram & datagram){
    if(!ex && 0 != datagram.data_size()){
      if(datagram.data()[0] == message_type_t::Handshake){
        this->sessions_[datagram.address()] = std::make_shared<dht_session>();
      }
      else if(datagram.data()[0] == message_type_t::Welcome){
        this->sessions_[datagram.address()] = std::make_shared<dht_session>();
      }
      else {
        auto p = this->sessions_.find(datagram.address());
        if(this->sessions_.end() != p){
          p.second->process_datagram(sp, s, datagram)
              .execute([](const std::shared_ptr<std::exception> & ex){
                if(ex){
                  this->sessions_.remove(datagram.address());
                }
                else {
                  pthis->continue_read(sp, s);
                }
              });
          return;
        }
      }
    }

    pthis->continue_read(sp, s);

  });
}
