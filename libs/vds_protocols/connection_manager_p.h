#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class connection_manager;
  
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
      const const_data_buffer & binary_form,
      const std::string & json_form);

    event_source<const const_data_buffer &> & incoming_message(uint32_t message_type_id);
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
        const const_data_buffer & binary_form);

    private:
      _connection_manager * owner_;
      service_provider sp_;
      udp_socket s_;
      logger log_;
      lazy_service<istorage_log> storage_log_;
      pipeline<std::string, uint16_t, const_data_buffer> message_queue_;

      class out_session_data
      {
      public:
        out_session_data(
          const std::string & original_server,
          uint16_t original_port);

        void init_session(
          uint32_t external_session_id,
          const std::string & real_server,
          uint16_t real_port,
          certificate && cert,
          symmetric_key && session_key);

        const std::string & original_server() const { return this->original_server_; }
        uint16_t original_port() const { return this->original_port_; }
        uint32_t external_session_id() const { return this->external_session_id_; }
        const std::string & real_server() const { return this->real_server_; }
        uint16_t real_port() const { return this->real_port_; }
        const certificate * cert() const { return this->cert_.get(); }
        const symmetric_key * session_key() const { return this->session_key_.get(); }

      private:
        std::string original_server_;
        uint16_t original_port_;
        uint32_t external_session_id_;
        std::string real_server_;
        uint16_t real_port_;
        std::unique_ptr<certificate> cert_;
        std::unique_ptr<symmetric_key> session_key_;
      };
      std::mutex out_sessions_mutex_;
      std::map<uint32_t, std::unique_ptr<out_session_data>> out_sessions_;
      
      class in_session_data
      {
      public:
        in_session_data(
          const std::string & server,
          uint16_t port,
          uint32_t session_id,
          const guid & server_id,
          const symmetric_key & session_key);

        const std::string & server() const { return this->server_; }
        uint16_t port() const { return this->port_; }
        uint32_t session_id() const { return this->session_id_; }
        const guid & server_id() const { return this->server_id_;}
        const symmetric_key & session_key() const { return this->session_key_; }
        
      private:
        std::string server_;
        uint16_t port_;
        uint32_t session_id_;
        guid server_id_;
        symmetric_key session_key_;
      };
      std::mutex in_sessions_mutex_;
      std::map<uint32_t, std::unique_ptr<in_session_data>> in_sessions_;
      
      void for_each_connection(const std::function<void(
        uint32_t session_id,
        const symmetric_key & session_key,
        const std::string & server,
        uint16_t port)> & callback);
    };
    
    std::unique_ptr<udp_server> udp_server_;
    std::map<uint32_t, event_source<const const_data_buffer &>> input_message_handlers_;
  };
}

#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
