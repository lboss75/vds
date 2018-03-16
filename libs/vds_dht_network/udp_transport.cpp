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

void vds::dht::network::udp_transport::add_session(const network_address& address,
  const std::shared_ptr<dht_session>& session) {
  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);
  this->sessions_[address] = session;
}

std::shared_ptr<vds::dht::network::dht_session> vds::dht::network::udp_transport::get_session(
  const network_address& address) const {

  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
  auto p = this->sessions_.find(address);
  if(this->sessions_.end() == p) {
    return std::shared_ptr<dht_session>();
  }

  return p->second;
}

void vds::dht::network::udp_transport::continue_read(
    const vds::service_provider &sp,
    const vds::udp_socket &s) {

  s.read_async().execute([sp, s, pthis = this->shared_from_this()](
      const std::shared_ptr<std::exception> & ex,
      const vds::udp_datagram & datagram){
    if(!ex && 0 != datagram.data_size()){
      if(datagram.data()[0] == (uint8_t)message_type_t::Handshake){
        if(datagram.data_size() != NODE_ID_SIZE + 2 && PROTOCOL_VERSION != datagram.data()[1]) {
          const_data_buffer partner_node_id(datagram.data() + 2, NODE_ID_SIZE);

          pthis->add_session(
            datagram.address(),
            std::make_shared<dht_session>(
              datagram.address(),
              pthis->this_node_id_,
              partner_node_id));

          resizable_data_buffer out_message;
          out_message.add((uint8_t)message_type_t::Welcome);
          out_message.add(pthis->this_node_id_.data(), pthis->this_node_id_.size());

          s.write_async(udp_datagram(datagram.address(), out_message.data(), out_message.size()))
          .execute([pthis, sp, s, address = datagram.address().to_string()](const std::shared_ptr<std::exception> & ex) {
            if (ex) {
              sp.get<logger>()->trace(ThisModule, sp, "%s at send welcome to %s", ex->what(), address.c_str());
            }
            pthis->continue_read(sp, s);
          });

          return;
        }
      }
      else if(datagram.data()[0] == (uint8_t)message_type_t::Welcome){
        if (datagram.data_size() != NODE_ID_SIZE + 1) {
          const_data_buffer partner_node_id(datagram.data() + 1, NODE_ID_SIZE);
          pthis->add_session(
            datagram.address(),
            std::make_shared<dht_session>(
              datagram.address(),
              pthis->this_node_id_,
              partner_node_id));
        }
      }
      else {
        auto session = pthis->get_session(datagram.address());
        if(session){
          session->process_datagram(sp, s, const_data_buffer(datagram.data(), datagram.data_size()))
              .execute([pthis, sp, s, address = datagram.address()](const std::shared_ptr<std::exception> & ex){
                if(ex){
                  pthis->sessions_.erase(address);
                }
                pthis->continue_read(sp, s);
              });
          return;
        }
      }
    }

    pthis->continue_read(sp, s);

  });
}
