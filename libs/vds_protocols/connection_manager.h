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
    void broadcast(const service_provider & sp, const message_type & message)
    {
      binary_serializer b;
      message.serialize(b);
      
      this->broadcast(
        sp,
        message_type::message_type_id,
        b.data());
    }

    template<typename message_type>
    void send_to(
      const service_provider & sp,
      const connection_session & session,
      const message_type & message)
    {
      binary_serializer b;
      message.serialize(b);
      
      this->send_to(
        sp,
        session,
        message_type::message_type_id,
        b.data());
    }

    template<typename message_type>
    void send_to(
      const service_provider & sp,
      const guid & server_id,
      const message_type & message)
    {
      binary_serializer b;
      message.serialize(b);
      
      this->send_to(
        sp,
        server_id,
        message_type::message_type_id,
        b.data());
    }

    void send_transfer_request(
      const service_provider & sp,
      const guid & server_id,
      uint64_t index);

    const _connection_manager * operator -> () const;
    _connection_manager * operator -> ();
    
  private:
    void broadcast(
      const service_provider & sp,
      uint32_t message_type_id,
      const const_data_buffer & message_data);

    void send_to(
      const service_provider & sp,
      const connection_session & session,
      uint32_t message_type_id,
      const const_data_buffer & message_data);

    void send_to(
      const service_provider & sp,
      const guid & server_id,
      uint32_t message_type_id,
      const const_data_buffer & message_data);
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

    _connection_manager * operator -> () const { return this->impl_.get(); }
  private:
    std::unique_ptr<_connection_manager> impl_;
  };
}
#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
