//
// Created by vadim on 26.11.17.
//

#include <guid.h>
#include <const_data_buffer.h>
#include "p2p_network_client.h"

vds::async_task<> vds::p2p_network_client::process_message(const guid &user_id, const const_data_buffer &message) {
  throw std::runtime_error("Not implemented");
}
