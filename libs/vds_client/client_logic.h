#ifndef __VDS_CLIENT_CLIENT_LOGIC_H_
#define __VDS_CLIENT_CLIENT_LOGIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client_connection.h"

namespace vds {
  class client_logic
  {
  public:
    client_logic(
      const service_provider & sp,
      certificate & client_certificate,
      asymmetric_private_key & client_private_key
    );

    void start();
    void stop();

    void connection_closed(client_connection<client_logic> * connection);
    void connection_error(client_connection<client_logic> * connection, std::exception * ex);

    template <typename message_type>
    task_job send_message(const message_type & message);

  private:
    const service_provider & sp_;
    certificate & client_certificate_;
    asymmetric_private_key & client_private_key_;

    std::vector<client_connection<client_logic> *> connection_queue_;

    static constexpr size_t MAX_CONNECTIONS = 10;
    std::mutex connection_mutex_;
    size_t connected_;

    void update_connection_pool();

    template <typename message_type>
    class send_message_job
    {
    public:
      send_message_job(client_logic * owner, const message_type & message)
        : owner_(owner), message_(message)
      {
      }

      void operator()()
      {
      }

    private:
      client_logic * owner_;
      message_type message_;
    };
  };


  template<typename message_type>
  inline task_job client_logic::send_message(const message_type & message)
  {
    auto task_manager = this->sp_.get<itask_manager>();
    auto job = task_manager.create_job(send_message_job<message_type>(this, message));
    job.start();
    return job;
  }
}


#endif // __VDS_CLIENT_CLIENT_LOGIC_H_
