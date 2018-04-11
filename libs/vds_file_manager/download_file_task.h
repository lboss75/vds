#ifndef __VDS_FILE_MANAGER_DOWNLOAD_FILE_TASK_H_
#define __VDS_FILE_MANAGER_DOWNLOAD_FILE_TASK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <file.h>
#include <vds_debug.h>
#include "transactions/file_add_transaction.h"
#include "guid.h"
#include "filename.h"
#include "async_buffer.h"

namespace vds {
  namespace file_manager {
    class download_file_task {
      public:

      download_file_task(
          const const_data_buffer & channel_id,
          const const_data_buffer & target_file,
          const std::shared_ptr<async_buffer<uint8_t>> & target_stream)
          : channel_id_(channel_id),
            target_file_(target_file),
            target_stream_(target_stream),
            body_size_(0) {
      }

      struct block_info {
        transactions::file_add_transaction::file_block_t id_;
        size_t offset_;
        bool is_processed_;

        block_info(
            const transactions::file_add_transaction::file_block_t & id,
            size_t offset)
        : id_(id),
          offset_(offset),
          is_processed_(false) {

        }
      };

      const const_data_buffer & channel_id() const {
        return this->channel_id_;
      }

      const std::string & name() const {
        return this->name_;
      }

      const const_data_buffer & target_file() const {
        return this->target_file_;
      }

      const std::string & mime_type() const {
        return this->mime_type_;
      }


      const std::list<block_info> & file_blocks() const {
        return this->file_blocks_;
      }

      std::list<block_info> & file_blocks() {
        return this->file_blocks_;
      }

      void set_file_blocks(
          const std::string & mime_type,
          const std::list<transactions::file_add_transaction::file_block_t> & file_blocks){
        this->mime_type_ = mime_type;
        size_t offset = 0;
        for(auto & p : file_blocks) {
          this->file_blocks_.push_back(block_info(p, offset));
          offset += p.block_size;
        }
      }

      void set_file_data(
          block_info & block,
          const const_data_buffer & data){

        vds_assert(block.id_.block_size == data.size());

        block.is_processed_ = true;

        file f(this->target_file_, file::file_mode::open_or_create);
        f.seek(block.offset_);
        f.write(data.data(), block.id_.block_size);
        f.close();

      }


      size_t body_size() const {
        return this->body_size_;
      }

    private:
      const_data_buffer channel_id_;
      std::string name_;
      const_data_buffer target_file_;
      std::string mime_type_;
      std::list<block_info> file_blocks_;
      std::shared_ptr<async_buffer<uint8_t>> target_stream_;
      size_t body_size_;
    };
  }
}

#endif //__VDS_FILE_MANAGER_DOWNLOAD_FILE_TASK_H_
