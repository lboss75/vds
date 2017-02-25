#ifndef __VDS_PROTOCOLS_VSR_PROTOCOL_H_
#define __VDS_PROTOCOLS_VSR_PROTOCOL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "vsr_messages.h"

namespace vds {
  class vsr_protocol
  {
  public:
    
    class server_queue
    {
    public:
      virtual void new_client() = 0;
    };

    //interface
    class iserver_queue
    {
    public:
      iserver_queue(server_queue * owner)
      : owner_(owner)
      {
      }
      
      void new_client() { this->owner_->new_client(); }
      
      void add_task(std::unique_ptr<json_object> && message);
      
    public:
      server_queue * owner_;
    };

    class client;
    class iclient
    {
    public:
      iclient(client * owner);
      
      using client_id_assigned_handler_t = std::function<void (client & sender)>;
      void subscribe_client_id_assigned(const client_id_assigned_handler_t & handler);
      
      void get_messages(json_writer & writer);
      
    private:
      client * owner_;
    };

    class client
    {
    public:
      client(const service_provider & sp);

      void start();
      void new_client_request_complete(const vsr_new_client_message_complete & response);

    private:
      friend class iclient;
      
      service_provider sp_;
      logger log_;
      size_t last_request_number_;
      size_t client_id_;
      size_t current_primary_view_;
      size_t server_count_;
      itask_manager task_manager_;
      iserver_queue server_queue_;
      std::mutex lock_mutex_;
      std::list<iclient::client_id_assigned_handler_t> client_id_assigned_handlers_;

      void client_id_assigned();
      void get_messages(json_writer & writer);
    };
    
    class server;
    class iserver
    {
    public:
      iserver(server * owner);
      
      void start_standalone();
      
    private:
      server * owner_;      
    };

    class server
    {
    public:
      server(const service_provider & sp);
      ~server();

      void start();
      void new_client();

    private:
      friend class iserver;
      
      struct replica_connection
      {

      };
      
      struct client_connection
      {
        size_t last_request_number_;

      };

      enum server_state
      {
        normal,
        view_change,
        recovering
      };

      service_provider sp_;
      logger log_;
      size_t last_request_number_;
      size_t client_id_;
      size_t current_primary_view_;
      size_t server_count_;
      itask_manager task_manager_;
      std::mutex lock_mutex_;
      
      std::map<long, replica_connection> replicas_;
      std::map<long, client_connection> clients_;
      size_t last_commit_number_;
      
      void start_standalone();
    };
  };
}


#endif // __VDS_PROTOCOLS_VSR_PROTOCOL_H_
