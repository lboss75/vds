#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class connection_manager;

  class connection_session : public std::enable_shared_from_this<connection_session>
  {
  public:
    virtual void send_to(
      uint32_t message_type_id,
      const std::function<const_data_buffer(void)> & get_binary,
      const std::function<std::string(void)> & get_json) const = 0;
  };

  class _connection_manager
  {
  public:
    _connection_manager(
      const vds::service_provider& sp,
      connection_manager * owner);
    ~_connection_manager();

    void start();
    void stop();

    void start_servers(const std::string & server_addresses);
    
    void broadcast(
      uint32_t message_type_id,
      const std::function<const_data_buffer(void)> & get_binary,
      const std::function<std::string(void)> & get_json);

    event_source<const connection_session &, const const_data_buffer &> & incoming_message(uint32_t message_type_id);

    void send_to(
      const connection_session & session,
      uint32_t message_type_id,
      const std::function<const_data_buffer(void)> & get_binary,
      const std::function<std::string(void)> & get_json)
    {
      session.send_to(message_type_id, get_binary, get_json);
    }

  private:
    service_provider sp_;
    connection_manager * const owner_;
    logger log_;
        
    async_task<> start_udp_server(const url_parser::network_address& address);
    async_task<> start_https_server(const url_parser::network_address& address);
    
    class udp_server
    {
    public:
      udp_server(_connection_manager * owner);
      async_task<> start(const url_parser::network_address& address);
      
      //UDP server handlers
      void socket_closed(std::list<std::exception_ptr> errors);
      async_task<> input_message(const sockaddr_in * from, const void * data, size_t len);
      
      template <typename next_step_type>
      void get_message(next_step_type & next)
      {
        this->message_queue_.get(next);
      }
      
      void open_udp_session(const std::string & address);

      void broadcast(
        uint32_t message_type_id,
        const std::function<const_data_buffer(void)> & get_binary,
        const std::function<std::string(void)> & get_json);

    private:
      _connection_manager * owner_;
      service_provider sp_;
      udp_socket s_;
      logger log_;
      lazy_service<istorage_log> storage_log_;
      pipeline<std::string, uint16_t, const_data_buffer> message_queue_;

      class hello_request
      {
      public:
        hello_request();

        hello_request(
          uint32_t session_id,
          const std::string & server,
          uint16_t port);

        uint32_t session_id() const { return this->session_id_; }
        const std::string & server() const { return this->server_; }
        uint16_t port() const { return this->port_; }

      private:
        uint32_t session_id_;
        std::string server_;
        uint16_t port_;
      };

      std::mutex hello_requests_mutex_;
      std::map<uint32_t, hello_request> hello_requests_;

      class session : public connection_session
      {
      public:
        session(
          udp_server * owner,
          uint32_t session_id,
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

        void send_to(
          uint32_t message_type_id,
          const std::function<const_data_buffer(void)> & get_binary,
          const std::function<std::string(void)> & get_json) const override;

      private:
        udp_server * const owner_;
        uint32_t session_id_;
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
          udp_server * owner,
          const hello_request & original_request,
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
          udp_server * owner,
          uint32_t session_id,
          const std::string & server,
          uint16_t port,
          uint32_t external_session_id,
          const guid & server_id,
          const symmetric_key & session_key);
      };

      std::mutex sessions_mutex_;
      std::map<uint32_t, std::shared_ptr<session>> sessions_;

      const incoming_session & register_incoming_session(
        const std::string & server,
        uint16_t port,
        uint32_t external_session_id,
        const guid & server_id);
      
      void for_each_sessions(const std::function<void(const session &)> & callback);
      void process_timer_jobs();
    };
    
    std::unique_ptr<udp_server> udp_server_;
    std::map<uint32_t, event_source<const connection_session & , const const_data_buffer &>> input_message_handlers_;

  };
}

#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
