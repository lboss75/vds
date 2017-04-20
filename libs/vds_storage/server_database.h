#ifndef __VDS_STORAGE_SERVER_DATABASE_H_
#define __VDS_STORAGE_SERVER_DATABASE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_manager.h"

namespace vds {
  class _server_database;
  class cert;

  class server_database
  {
  public:
    server_database(const service_provider & sp);
    ~server_database();

    void start();
    void stop();

  private:
    friend class iserver_database;
    _server_database * const impl_;
  };

  class iserver_database
  {
  public:
    iserver_database(server_database * owner);

    void add_cert(const cert & record);
    std::unique_ptr<cert> find_cert(const std::string & object_name) const;

    void add_object(
      const guid & server_id,
      const server_log_new_object & index);

    void add_file(
      const guid & server_id,
      const server_log_file_map & fm);

    uint64_t last_object_index(
      const guid & server_id);

    void add_endpoint(
      const std::string & endpoint_id,
      const std::string & addresses);

    void get_endpoints(std::map<std::string, std::string> & addresses);

    uint64_t get_server_log_max_index(const guid & id);

    server_log_record add_local_record(
      const server_log_record::record_id & record_id,
      const json_value * message,
      const_data_buffer & signature);

    //return: true - saved, false - already exists
    bool save_record(
      const server_log_record & record,
      const const_data_buffer & signature);

    enum class server_log_state
    {
      not_found = 0,
      stored = 1, //Just stored
      front = 2, //Stored + all parents processed
      processed = 3, //+ Processed
      tail = 4 //
    };

    void get_unknown_records(std::list<server_log_record::record_id> & result);

    bool get_record(
      const server_log_record::record_id & id,
      server_log_record & result_record,
      const_data_buffer & result_signature);

  private:
    server_database * const owner_;
  };
}

#endif // __VDS_STORAGE_SERVER_DATABASE_H_
