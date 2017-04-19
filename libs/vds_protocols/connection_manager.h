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
  
  class iconnection_manager
  {
  public:
    iconnection_manager(_connection_manager * owner);
    
    template<typename message_type>
    void broadcast(const message_type & message)
    {
      binary_serializer b;
      message.serialize(b);

      this->broadcast(
        message_type::message_type_id,
        b.data(),
        message.serialize()->str());
    }

    event_source<const const_data_buffer &> & incoming_message(uint32_t message_type_id);

  private:
    _connection_manager * const owner_;

    void broadcast(
      uint32_t message_type_id,
      const const_data_buffer & binary_form,
      const std::string & json_form);
  };
}
#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
