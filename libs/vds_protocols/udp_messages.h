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
      hello_message_id = 1,
      welcome_message_id = 2,
      command_message_id = 3
    };
    
    class hello_message
    {
    public:
      hello_message(
        const std::string & source_certificate,
        uint32_t generation_id,
        const std::string & to_url);
      
      hello_message(network_deserializer& s);
      
      const std::string & source_certificate() const { return this->source_certificate_; }
      uint32_t generation_id() const { return this->generation_id_; }
      const std::string & to_url() const { return this->to_url_; }
      
      void serialize(network_serializer& s) const;
      
    private:
      std::string source_certificate_;
      uint32_t generation_id_;
      std::string to_url_;
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
