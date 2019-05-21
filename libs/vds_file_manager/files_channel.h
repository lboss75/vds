#ifndef __VDS_FILE_MANAGER_FILES_CHANNEL_H_
#define __VDS_FILE_MANAGER_FILES_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "filename.h"
#include <optional>
#include "file_operations.h"

namespace vds {
  class database_read_transaction;
  class user_manager;
}

namespace vds {
  namespace file_manager {
    class file_in_storage {
    public:

      file_in_storage(
        const const_data_buffer & id,
        const filename & name,
        const std::string & mime_type,
        uint64_t size
      );

      const const_data_buffer& id() const {
        return id_;
      }

      const filename& name() const {
        return name_;
      }

      const std::string& mime_type() const {
        return mime_type_;
      }

      uint64_t size() const {
        return size_;
      }

    private:
      const_data_buffer id_;
      filename name_;
      std::string mime_type_;
      uint64_t size_;
    };

    class files_channel : public std::enable_shared_from_this<files_channel> {
    public:
      files_channel(
        const service_provider * sp,
        const std::shared_ptr<user_manager> & user_manager,
        const const_data_buffer & channel_id);

      expected<std::optional<file_in_storage>> looking_last_file(
        database_read_transaction & t,
        const filename name,
        const const_data_buffer file_hash);

      async_task<vds::expected<vds::file_manager::file_operations::download_result_t>> download_file(
        const filename & file_name,
        const const_data_buffer & file_hash);


    private:
      const service_provider * sp_;
      std::shared_ptr<user_manager> user_manager_;
      const_data_buffer channel_id_;
    };
  }
}

#endif //__VDS_FILE_MANAGER_FILES_CHANNEL_H_
