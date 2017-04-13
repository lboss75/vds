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
      
      const_data_buffer serialize() const;
      
    private:
      std::string source_certificate_;
      uint32_t session_id_;
      std::string to_url_;
    };

    class welcome_message
    {
    public:
      welcome_message(
        const std::string & server_certificate,
        const const_data_buffer & key_crypted,
        const const_data_buffer & crypted_data,
        const const_data_buffer & sign);

      welcome_message(network_deserializer& s);
      const_data_buffer serialize() const;

      const std::string & server_certificate() const { return this->server_certificate_; }
      const const_data_buffer & key_crypted() const { return this->key_crypted_; }
      const const_data_buffer & crypted_data() const { return this->crypted_data_; }
      const const_data_buffer & sign() const { return this->sign_; }

    private:
      std::string server_certificate_;
      const_data_buffer key_crypted_;
      const_data_buffer crypted_data_;
      const_data_buffer sign_;
    };
  }
}

#endif // __VDS_PROTOCOLS_UDP_MESSAGES_H_
