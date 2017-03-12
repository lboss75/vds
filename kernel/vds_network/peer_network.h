#ifndef __VDS_NETWORK_PEER_NETWORK_H_
#define __VDS_NETWORK_PEER_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "peer_channel.h"
#include "network_serializer.h"

namespace vds {
  class _peer_network;
  
  class peer_network
  {
  public:
    peer_network(const service_provider & sp);
    ~peer_network();


    template <typename message_type>
    void broadcast(const message_type & message)
    {
      this->for_each_active_channel([&message](peer_channel * channel) {
        switch (channel->get_formatter_type()) {
        case peer_channel::binary:
        {
          network_serializer s;
          message.serialize(s);

          channel->broadcast(s.data());
          break;
        }

        case peer_channel::json:
        {
          channel->broadcast(message.serialize()->str());
          break;
        }

        default:
          throw new std::logic_error("Invalid login");
        }
      });
    }
    
    template <typename message_type>
    class _handle_message
    {
    public:
      _handle_message(peer_network * owner);
      
      template <typename message_handler>
      void with(message_handler & handler)
      {
        this->owner_->register_handler(
          new message_handler(
            message_type::message_type,
            message_type::message_type_id,
            handler));
      }
      
    private:
      peer_network * owner_;
    };
    
    template <typename message_type>
    _handle_message<message_type> handle_message();

  private:
    friend class _peer_network;
    _peer_network * impl_;
    
    class message_handler_base
    {
    public:
      message_handler_base(
        const std::string & json_message_type,
        uint8_t binary_message_type        
      );
      
      const std::string & json_message_type() const { return this->json_message_type_; }
      uint8_t binary_message_type() const {return this->binary_message_type_; }
      
      virtual void process_json(peer_channel * channel, const json_value * body) = 0;
      virtual void process_binary(peer_channel * channel, network_deserializer & body) = 0;
      
    private:
      std::string json_message_type_;
      uint8_t binary_message_type_;      
    };
    
    template <typename message_type, typename message_handler_type>
    class message_handler : public message_handler_base
    {
    public:
      message_handler(
        const std::string & json_message_type,
        uint8_t binary_message_type,
        message_handler_type & hanlder         
      );
     
      void process_json(peer_channel * channel, const json_value * body) override;
      void process_binary(peer_channel * channel, network_deserializer & body) override;
      
    private:
      message_handler_type & hanlder_;
    };

    void for_each_active_channel(const std::function<void(peer_channel *)> & callback);
    void register_handler(message_handler_base * handler);
  };

  template <typename message_type>
  peer_network::_handle_message<message_type>::_handle_message(peer_network * owner)
  : owner_(owner)
  {
  }
  
/*  template <typename message_type>
  template <typename message_handler>
  void peer_network::_handle_message<message_type>::with<message_handler>(message_handler & handler)
  {
    this->owner_->register_handler(
      new message_handler(
        message_type::message_type,
        message_type::message_type_id,
        handler);
  }*/
  
  template <typename message_type, typename message_handler_type>
  peer_network::message_handler<message_type, message_handler_type>::message_handler(
    const std::string & json_message_type,
    uint8_t binary_message_type,
    message_handler_type & hanlder         
  ) : message_handler_base(json_message_type, binary_message_type)
  {
  }
     
  template <typename message_type, typename message_handler_type>
  void peer_network::message_handler<message_type, message_handler_type>::process_json(
    peer_channel * channel,
    const json_value * body)
  {
    this->handler_(channel, message_type(body));
  }
  
  template <typename message_type, typename message_handler_type>
  void peer_network::message_handler<message_type, message_handler_type>::process_binary(
    peer_channel * channel,
    network_deserializer & body)
  {
    this->handler_(channel, message_type(body));
  }
}

#endif//__VDS_NETWORK_PEER_NETWORK_H_