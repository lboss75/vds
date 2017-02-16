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
      
    public:
      server_queue * owner_;
    };

    class iclient
    {
    public:

    };

    class client
    {
    public:
      client(const service_provider & sp);

      void start();
      void new_client_request_complete(const vsr_new_client_message_complete & response);

    private:
      service_provider sp_;
      logger log_;
      size_t last_request_number_;

    protected:
      size_t client_id_;
      size_t current_primary_view_;
      size_t server_count_;
      itask_manager task_manager_;
      iserver_queue server_queue_;

      std::mutex lock_mutex_;

      virtual void client_id_assigned();
    };

    class server : public client
    {
    public:
      server(const service_provider & sp);

      void start();
      void new_client();

    private:
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

      logger log_;
      std::function<void(void)> get_client_id_timeout_;
      task_job get_client_id_task_;

      std::map<long, replica_connection> replicas_;
      std::map<long, client_connection> clients_;
      size_t last_request_number_;
      size_t last_commit_number_;

      void get_client_id_timeout();
    };
  };
}


#endif // __VDS_PROTOCOLS_VSR_PROTOCOL_H_
