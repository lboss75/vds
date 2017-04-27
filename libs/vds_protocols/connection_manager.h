#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class connection_session;
  class _connection_manager;

  class iconnection_manager
  {
  public:
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

    async_task<> download_object(
      const guid & server_id,
      uint64_t index,
      const filename & target_file);

  private:
    void broadcast(
      uint32_t message_type_id,
      const std::function<const_data_buffer(void)> & get_binary,
      const std::function<std::string(void)> & get_json);

    void send_to(
      const connection_session & session,
      uint32_t message_type_id,
      const std::function<const_data_buffer(void)> & get_binary,
      const std::function<std::string(void)> & get_json);
  };


  class connection_manager : public iservice_factory
  {
  public:
    connection_manager();
    ~connection_manager();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

    void start_servers(
      const service_provider & sp,
      const std::string & server_addresses);

  private:
    std::unique_ptr<_connection_manager> impl_;
  };
}
#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
