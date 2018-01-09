/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <transactions/channel_add_message_transaction.h>
#include "cert_control.h"
#include "stdafx.h"
#include "file_operations.h"
#include "file.h"
#include "db_model.h"
#include "private/file_operations_p.h"
#include "chunk_manager.h"
#include "service_provider.h"
#include "vds_debug.h"
#include "transaction_block.h"
#include "transactions/file_add_transaction.h"
#include "user_manager.h"
#include "member_user.h"

vds::file_manager::file_operations::file_operations()
: impl_(new file_manager_private::_file_operations()){
}

vds::async_task<> vds::file_manager::file_operations::upload_file(
    const service_provider &sp,
    const guid &channel_id,
    const std::string &name,
    const std::string &mimetype,
    const vds::filename &file_path) {
  return this->impl_->upload_file(sp, channel_id, name, mimetype, file_path);
}

//////////////////////////////////////////////////////////////////////////////////////////////
vds::async_task<> vds::file_manager_private::_file_operations::upload_file(
    const service_provider & sp,
    const guid & channel_id,
    const std::string &name,
    const std::string &mimetype,
    const vds::filename &file_path) {

  return sp.get<db_model>()->async_transaction(
      sp,
      [pthis = this->shared_from_this(),
          sp,
          name,
          mimetype,
          file_path,
          channel_id](
          database_transaction & t) {

        auto user_mng = sp.get<user_manager>();

        asymmetric_private_key device_private_key;
        auto user = user_mng->get_current_device(sp, t, device_private_key);

        auto channel = user_mng->get_channel(t, channel_id);
        auto channel_write_key = user_mng->get_channel_write_key(
            sp, t, channel, user.id());

        std::list<transactions::file_add_transaction::file_block_t> file_blocks;
        pthis->pack_file(sp, file_path, t, file_blocks);

        transactions::transaction_block log;
        log.add(
            channel_id,
            transactions::file_add_transaction(
                channel.write_cert(),
                user_mng->get_channel_write_key(sp, t, channel, user.id()),
                name,
                mimetype,
                file_blocks));

        auto common_channel = user_mng->get_common_channel(t);

        log.save(
            sp,
            t,
            common_channel.read_cert(),
            user.user_certificate(),
            device_private_key);
      }
  );
}

void vds::file_manager_private::_file_operations::pack_file(
    const vds::service_provider &sp,
    const vds::filename &file_path,
    vds::database_transaction &t,
    std::list<transactions::file_add_transaction::file_block_t> &file_blocks) const {
  auto chunk_mng = sp.get<vds::chunk_manager>();
  auto file_size = vds::file::length(file_path);
  if (file_size > 0) {
    if (file_size < vds::file_manager::file_operations::BLOCK_SIZE) {
      auto buffer = (uint8_t *) alloca(file_size);
      vds::file f(file_path, vds::file::file_mode::open_read);

      auto readed = f.read(buffer, file_size);
      vds_assert(readed == file_size);
      auto block_info = chunk_mng->save_block(t, vds::const_data_buffer(buffer, readed));
      file_blocks.push_back(
          transactions::file_add_transaction::file_block_t
          {
              .block_id = block_info.id,
              .block_key = block_info.key
          });
    } else {
      auto count = file_size / vds::file_manager::file_operations::BLOCK_SIZE;
      auto block_size = (file_size + count - 1) / count;
      auto buffer = (uint8_t *) alloca(block_size);
      vds::file f(file_path, vds::file::file_mode::open_read);

      for (uint64_t offset = 0; offset < file_size; offset += block_size) {
        auto readed = f.read(buffer, block_size);
        auto block_info = chunk_mng->save_block(t, vds::const_data_buffer(buffer, readed));
        file_blocks.push_back(
            transactions::file_add_transaction::file_block_t
                {
                    .block_id = block_info.id,
                    .block_key = block_info.key
                });
      }
    }
  }
}

