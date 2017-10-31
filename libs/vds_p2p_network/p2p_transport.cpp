//
// Created by vadim on 31.10.17.
//

#include "p2p_transport.h"
#include "private/p2p_transport_p.h"
#include "binary_serialize.h

static constexpr uint8_t protocol_version = 0;
///////////////////////////////////////////////////////
vds::const_data_buffer vds::_p2p_transport::next_message() {
  binary_serializer data;
  switch(this->current_state_){
    case send_state::bof:
      data << (uint32_t)(static_cast<uint32_t>(control_type::handshake) << 24);
      data << protocol_version;
      data << (uint16_t)this->mtu_;
      break;
  }

  return data.data();
}

