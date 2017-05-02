#ifndef __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_
#define __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "connection_manager.h"
#include "log_records.h"

namespace vds {
  
  class _server_log_sync
  {
  public:
    _server_log_sync(
      server_log_sync * owner);
    
    ~_server_log_sync();
    
    void start(const service_provider & sp);
    void stop(const service_provider & sp);

  private:
    friend class _storage_log;
    server_log_sync * const owner_;
    timer timer_;
    
    void on_new_local_record(
      const service_provider & sp,
      const server_log_record & record,
      const const_data_buffer & signature);

    class server_log_record_broadcast
    {
    public:
      static const char message_type[];
      static const uint32_t message_type_id;

      server_log_record_broadcast(
        const server_log_record & record,
        const const_data_buffer & signature);

      server_log_record_broadcast(
        const service_provider & sp,
        const const_data_buffer & data);

      void serialize(binary_serializer & b) const;
      std::unique_ptr<json_value> serialize() const;

      const server_log_record & record() const { return this->record_; }
      const const_data_buffer & signature() const { return this->signature_; }

    private:
      server_log_record record_;
      const_data_buffer signature_;
    };

    
    class server_log_get_records_broadcast
    {
    public:
      static const char message_type[];
      static const uint32_t message_type_id;

      server_log_get_records_broadcast(
        const std::list<server_log_record::record_id> & unknown_records);

      server_log_get_records_broadcast(
        const const_data_buffer & data);

      void serialize(binary_serializer & b) const;
      std::unique_ptr<json_value> serialize() const;

      const std::list<server_log_record::record_id> & unknown_records() const { return this->unknown_records_; }

    private:
      std::list<server_log_record::record_id> unknown_records_;
    };


    void on_record_broadcast(
      const service_provider & sp,
      const server_log_record_broadcast & message);
    void on_server_log_get_records_broadcast(
      const service_provider & sp,
      const connection_session & session,
      const server_log_get_records_broadcast & message);

    void require_unknown_records(const service_provider & sp);
    void process_timer_jobs(const service_provider & sp);
  };

}

#endif // __VDS_PROTOCOLS_SERVER_LOG_SYNC_P_H_
