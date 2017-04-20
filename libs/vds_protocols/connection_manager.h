#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _connection_manager;

  class connection_manager : public iservice
  {
  public:
    connection_manager();
    ~connection_manager();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

    void start_servers(const std::string & server_addresses);

  private:
    std::unique_ptr<_connection_manager> impl_;
  };
  
  class connection_session;

  class iconnection_manager
  {
  public:
    iconnection_manager(_connection_manager * owner);
    
    template<typename message_type>
    void broadcast(const message_type & message)
    {
      this->broadcast(
        message_type::message_type_id,
        [&message]() { binary_serializer b; message.serialize(b); return b.data(); },
        [&message]() { return message.serialize()->str(); });
    }

    template<typename message_type>
    void send_to(
      const connection_session & session,
      const message_type & message)
    {
      this->send_to(
        session,
        message_type::message_type_id,
        [&message]() { binary_serializer b; message.serialize(b); return b.data(); },
        [&message]() { return message.serialize()->str(); });
    }

    event_source<
      const connection_session &,
      const const_data_buffer &> & incoming_message(uint32_t message_type_id);

  private:
    _connection_manager * const owner_;

    void broadcast(
      uint32_t message_type_id,
      const std::function<const_data_buffer (void)> & get_binary,
      const std::function<std::string(void)> & get_json);

    void send_to(
      const connection_session & session,
      uint32_t message_type_id,
      const std::function<const_data_buffer(void)> & get_binary,
      const std::function<std::string(void)> & get_json);
  };
}
#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
