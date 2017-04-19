#ifndef __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_
#define __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "connection_manager.h"

namespace vds {
  
  class _server_log_sync
  {
  public:
    _server_log_sync(
      const service_provider & sp,
      server_log_sync * owner);
    
    ~_server_log_sync();
    
    void start();
    void stop();

  private:
    service_provider sp_;
    server_log_sync * const owner_;
    
    event_handler<
      const server_log_record & /*record*/,
      const const_data_buffer & /*signature*/> new_local_record_;

    event_handler<
      const const_data_buffer & /*binary_form */> record_broadcast_;

    lazy_service<iconnection_manager> connection_manager_;
      
    void on_new_local_record(const server_log_record & record, const const_data_buffer & signature);

    class server_log_record_broadcast
    {
    public:
      static const char message_type[];
      static const uint32_t message_type_id;

      server_log_record_broadcast(
        const server_log_record & record,
        const const_data_buffer & signature);

      server_log_record_broadcast(const const_data_buffer & data);

      void serialize(binary_serializer & b) const;
      std::unique_ptr<json_value> serialize() const;

      const server_log_record & record() const { return this->record_; }
      const const_data_buffer & signature() const { return this->signature_; }

    private:
      server_log_record record_;
      const_data_buffer signature_;
    };

    void on_record_broadcast(const server_log_record_broadcast & message);

  };

}

#endif // __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_
