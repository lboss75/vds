#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "simple_cache.h"
#include "object_transfer_protocol.h"
#include "route_manager.h"
#include "connection_manager.h"
#include "udp_socket.h"
#include "server_to_server_api.h"

namespace vds {
  class connection_session : public std::enable_shared_from_this<connection_session>
  {
  public:
    virtual const guid & server_id() const = 0;
    virtual const std::string & address() const = 0;

    virtual void send_to(
      const service_provider & sp,
      uint32_t message_type_id,
      const const_data_buffer & message_data) const = 0;
  };

  class _connection_manager : public iconnection_manager
  {
  public:
    _connection_manager(
      connection_manager * owner);

    ~_connection_manager();

    void start(const vds::service_provider& sp);
    void stop(const vds::service_provider& sp);

    void start_servers(
      const vds::service_provider& sp,
      const std::string & server_addresses);
    
    void broadcast(
      const service_provider & sp,
      uint32_t message_type_id,
      const const_data_buffer & message_data);

    void send_to(
      const service_provider & sp,
      const connection_session & session,
      uint32_t message_type_id,
      const const_data_buffer & message_data)
    {
      session.send_to(sp, message_type_id, message_data);
    }
    
    void send_to(
      const service_provider & sp,
      const guid & server_id,
      uint32_t message_type_id,
      const const_data_buffer & message_data);

    void send_transfer_request(
      const service_provider & sp,
      const guid & server_id,
      uint64_t index);
    
    void enum_sessions(
      const std::function<bool (connection_session &)> & callback);

    void possible_connections(
      const service_provider & sp,
      const std::list<trace_point> & trace_route);

  private:
    friend class server_to_server_api;
    friend class _route_manager;
    connection_manager * const owner_;
    route_manager route_manager_;
    server_to_server_api server_to_server_api_;
        
    void start_udp_channel(
      const vds::service_provider& sp,
      const url_parser::network_address& address);
    async_task<> start_https_server(
      const vds::service_provider& sp,
      const url_parser::network_address& address);
    
    class udp_channel
    {
    public:
      udp_channel(_connection_manager * owner);

      void start(
        const service_provider & sp,
        const url_parser::network_address& address);

      void stop(
        const service_provider & sp);

      //UDP server handlers
      //void socket_closed(
      //  const service_provider & sp,
      //  const std::list<std::shared_ptr<std::exception>> & errors);

      async_task<> input_message(
        const vds::service_provider& sp,
        const sockaddr_in * from,
        const void * data,
        size_t len);
      
      async_task<> open_udp_session(
        const service_provider & sp,
        const std::string & address);

      void broadcast(
        const service_provider & sp,
        uint32_t message_type_id,
        const const_data_buffer & message_data);

    private:
      friend class _connection_manager;
      _connection_manager * owner_;
      udp_server server_;
      
      udp_socket s_;
      timer process_timer_;

      class hello_request
      {
      public:
        hello_request();

        hello_request(
          uint32_t session_id,
          const std::string & address);

        uint32_t session_id() const { return this->session_id_; }
        const std::string & address() const { return this->address_; }

      private:
        uint32_t session_id_;
        std::string address_;
      };

      std::mutex hello_requests_mutex_;
      std::map<uint32_t, hello_request> hello_requests_;

      class session : public connection_session
      {
      public:
        session(
          udp_channel * owner,
          uint32_t session_id,
          const std::string & address,
          const std::string & server,
          uint16_t port,
          uint32_t external_session_id,
          const guid & partner_id,
          const symmetric_key & session_key);

        virtual ~session();

        uint32_t session_id() const { return this->session_id_; }
        const std::string & server() const { return this->server_; }
        uint16_t port() const { return this->port_; }
        uint32_t external_session_id() const { return this->external_session_id_; }
        const guid & partner_id() const { return this->partner_id_; }
        const symmetric_key & session_key() const { return this->session_key_; }

        const guid & server_id () const override { return this->partner_id_; }
        const std::string & address() const override { return this->address_; }

        void send_to(
          const service_provider & sp,
          uint32_t message_type_id,
          const const_data_buffer & message_data) const override;

      private:
        udp_channel * const owner_;
        uint32_t session_id_;
        std::string address_;
        std::string server_;
        uint16_t port_;
        uint32_t external_session_id_;
        guid partner_id_;
        symmetric_key session_key_;
      };

      class outgoing_session : public session
      {
      public:
        outgoing_session(
          udp_channel * owner,
          uint32_t session_id,
          const std::string & address,
          const std::string & server,
          uint16_t port,
          uint32_t external_session_id,
          const std::string & real_server,
          uint16_t real_port,
          certificate && cert,
          const symmetric_key & session_key);

        const std::string & real_server() const { return this->real_server_; }
        uint16_t real_port() const { return this->real_port_; }
        const certificate & cert() const { return this->cert_; }

      private:
        std::string real_server_;
        uint16_t real_port_;
        certificate cert_;
      };

      class incoming_session : public session
      {
      public:
        incoming_session(
          udp_channel * owner,
          uint32_t session_id,
          const std::string & server,
          uint16_t port,
          uint32_t external_session_id,
          const guid & server_id,
          const symmetric_key & session_key);
      };

      std::shared_mutex sessions_mutex_;
      std::map<uint32_t, std::shared_ptr<session>> sessions_;
      udp_datagram input_message_;

      const incoming_session & register_incoming_session(
        const std::string & server,
        uint16_t port,
        uint32_t external_session_id,
        const guid & server_id);
      
      void for_each_sessions(const std::function<bool (session &)> & callback);
      bool process_timer_jobs(const service_provider & sp);
      void schedule_read(const service_provider & sp);
    };
    
    std::unique_ptr<udp_channel> udp_channel_;
    object_transfer_protocol object_transfer_protocol_;

    async_task<> try_to_connect(
      const vds::service_provider& sp,
      const std::string & address);

  };
  
  inline _connection_manager * iconnection_manager::operator -> ()
  {
    return static_cast<_connection_manager *>(this);
  }
  inline const _connection_manager * iconnection_manager::operator -> () const
  {
    return static_cast<const _connection_manager *>(this);
  }

}

#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
