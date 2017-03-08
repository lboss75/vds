#ifndef __VDS_PROTOCOLS_UDP_MESSAGES_H_
#define __VDS_PROTOCOLS_UDP_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  namespace udp_messages {
    
    enum message_identification
    {
      invalid_message_id = 0,
      hello_message_id = 'h'
      
    };
    
    class hello_message
    {
    public:
      hello_message(
        const std::string & fingerprint
      );
      
      hello_message(
        vds::network_deserializer & s
      );
      
      void serialize(vds::network_serializer & s) const;
            
      
    private:
      std::string fingerprint_;
    };
  }
}

#endif // __VDS_PROTOCOLS_UDP_MESSAGES_H_
