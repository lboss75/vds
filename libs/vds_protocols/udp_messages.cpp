/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "udp_messages.h"

void vds::udp_messages::hello_message::serialize(vds::network_serializer& s) const
{
  s.start(hello_message_id);
  
  s << fingerprint_;
  
  s.final();

}

vds::udp_messages::hello_message::hello_message(vds::network_deserializer& s)
{
  s >> this->fingerprint_;
  
  s.final();
}
