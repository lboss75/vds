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
        uint32_t session_id,
        const std::string & to_url);
      
      hello_message(network_deserializer& s);
      
      const std::string & source_certificate() const { return this->source_certificate_; }
      uint32_t session_id() const { return this->session_id_; }
      const std::string & to_url() const { return this->to_url_; }
      
      void serialize(network_serializer& s) const;
      
    private:
      std::string source_certificate_;
      uint32_t session_id_;
      std::string to_url_;
    };

    class welcome_message
    {
    public:
      welcome_message(
        const guid & server_id,
        const data_buffer & key_crypted,
        const data_buffer & crypted_info,
        const data_buffer & sign);

      void serialize(network_serializer& s) const;

    private:
      guid server_id_;
      data_buffer key_crypted_;
      data_buffer crypted_info_;
      data_buffer sign_;
    };
  }
}

#endif // __VDS_PROTOCOLS_UDP_MESSAGES_H_
