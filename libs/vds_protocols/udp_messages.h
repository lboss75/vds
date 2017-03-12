#ifndef __VDS_PROTOCOLS_UDP_MESSAGES_H_
#define __VDS_PROTOCOLS_UDP_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  namespace udp_messages {
    
    struct hello_message
    {
      std::string from_server_id;
      std::string to_url;
    };

    struct welcome_message
    {
      std::string from_server_id;
      std::string client_url;
      std::string crypto_key;
    };
  }
}

#endif // __VDS_PROTOCOLS_UDP_MESSAGES_H_
