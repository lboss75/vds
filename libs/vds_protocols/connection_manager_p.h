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
    
    async_task<> start_server(const std::string& address);

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

      private:
        std::string original_server_;
        uint16_t original_port_;
        uint32_t external_session_id_;
        std::string real_server_;
        uint16_t real_port_;
        std::unique_ptr<certificate> cert_;
        std::unique_ptr<symmetric_key> session_key_;
      };
      std::map<uint32_t, std::unique_ptr<out_session_data>> out_sessions_;
      
      class in_session_data
      {
      public:
        in_session_data(
          const guid & server_id,
          const symmetric_key & session_key);

        const symmetric_key & session_key() const { return this->session_key_; }
      private:
        guid server_id_;
        symmetric_key session_key_;
      };
      
      std::map<uint32_t, std::unique_ptr<in_session_data>> in_sessions_;      
    };
  };
}

#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
