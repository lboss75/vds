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

    //interface
    class iserver
    {
    public:
      void new_client();

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

    protected:
      long client_id_;
      iserver server_;
      itask_manager task_manager_;

      long current_primary_view_;
      long min_consensus_;

      std::mutex lock_mutex_;

      virtual void client_id_assigned();
    };

    class server : public client
    {
    public:
      server(const service_provider & sp);

      void start();

    private:
      struct replica_connection
      {

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


      void get_client_id_timeout();
    };


  };

}


#endif // __VDS_PROTOCOLS_VSR_PROTOCOL_H_
