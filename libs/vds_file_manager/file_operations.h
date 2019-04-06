#ifndef __VDS_FILE_MANAGER_FILE_OPERATIONS_H_
#define __VDS_FILE_MANAGER_FILE_OPERATIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <string>
#include "user_message_transaction.h"
#include "const_data_buffer.h"
#include "json_object.h"
#include "async_buffer.h"
#include "dht_network_client.h"

namespace vds {
  class user_manager;

  namespace file_manager_private {
    class _file_operations;
  };

  namespace file_manager {
    class file_operations {
    public:
      struct download_result_t {
        const_data_buffer file_hash;
        std::string name;
        std::string mime_type;
        uint64_t size;
      };

      struct prepare_download_result_t {
        const_data_buffer file_hash;
        std::string name;
        std::string mime_type;
        uint64_t size;
        int progress;

        std::map<const_data_buffer, dht::network::client::block_info_t> blocks;

        std::shared_ptr<json_value> to_json() const {
          auto result = std::make_shared<json_object>();
          result->add_property("name", this->name);
          result->add_property("mime_type", this->mime_type);
          result->add_property("size", this->size);

          auto result_blocks = std::make_shared<json_array>();
          for(auto & p : this->blocks) {
            auto block = std::make_shared<json_object>();
            block->add_property("id", base64::from_bytes(p.first));

            auto replicas = std::make_shared<json_array>();
            for(auto & replica : p.second.replicas) {
              auto r = std::make_shared<json_object>();
              r->add_property("id", base64::from_bytes(replica.first));

              auto ra = std::make_shared<json_array>();
              for(auto index : replica.second) {
                ra->add(std::make_shared<json_primitive>(std::to_string(index)));
              }
              r->add_property("replicas", ra);
              replicas->add(r);
            }
            block->add_property("replicas", replicas);

            result_blocks->add(block);
          }
          result->add_property("blocks", result_blocks);
          result->add_property("progress", std::to_string(this->progress));
          return result;
        }
      };


      file_operations();

			vds::async_task<vds::expected<transactions::user_message_transaction::file_info_t>> upload_file(
        const std::shared_ptr<user_manager> & user_mng,
        const std::string & name,
        const std::string & mime_type,
        const std::shared_ptr<stream_input_async<uint8_t>> & input_stream,
        const const_data_buffer & file_hash);

      vds::async_task<vds::expected<void>> create_message(
        
        const std::shared_ptr<user_manager>& user_mng,
        const const_data_buffer& channel_id,
        const std::shared_ptr<json_value>& message,
        const std::list<transactions::user_message_transaction::file_info_t>& files);


			vds::async_task<vds::expected<download_result_t>> download_file(
			  const std::shared_ptr<user_manager> & user_mng,
			  const const_data_buffer & channel_id,
        const std::string & file_name,
        const const_data_buffer & file_hash,
        const std::shared_ptr<stream_output_async<uint8_t>> & output_stream);

      void start(const service_provider * sp);
      void stop();
      
      async_task<expected<void>> prepare_to_stop();

      async_task<expected<prepare_download_result_t>> prepare_download_file(
        const std::shared_ptr<user_manager> & user_mng,
        const const_data_buffer& channel_id,
        const std::string& file_name,
        const const_data_buffer& file_hash);

      async_task<expected<void>> download_stream(
        const std::shared_ptr<stream_output_async<uint8_t>> & target_stream,
        const std::list<transactions::user_message_transaction::file_block_t> &file_blocks);

    protected:
      std::shared_ptr<file_manager_private::_file_operations> impl_;
    };
  }
}

#endif //__VDS_FILE_MANAGER_FILE_OPERATIONS_H_
