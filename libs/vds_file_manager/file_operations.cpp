/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "file_operations.h"
#include "file.h"
#include "db_model.h"
#include "private/file_operations_p.h"
#include "service_provider.h"
#include "transaction_block.h"
#include "transactions/file_add_transaction.h"
#include "user_manager.h"
#include "vds_exceptions.h"
#include "chunk.h"
#include "logger.h"
#include "encoding.h"
#include "private/upload_stream_task_p.h"

vds::file_manager::file_operations::file_operations()
  : impl_(new file_manager_private::_file_operations()) {
}

vds::async_task<> vds::file_manager::file_operations::upload_file(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::string& name,
  const std::string& mimetype,
  const filename& file_path) {
  return this->impl_->upload_file(sp, user_mng, channel_id, name, mimetype, file_path);
}

vds::async_task<>
vds::file_manager::file_operations::download_file(
  const service_provider& sp,
  const std::shared_ptr<download_file_task>& task) {
  return this->impl_->download_file(sp, task);
}

vds::async_task<vds::const_data_buffer>
vds::file_manager::file_operations::upload_file(
  const service_provider& sp,
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::string& name,
  const std::string& mimetype,
  const std::shared_ptr<continuous_buffer<uint8_t>>& input_stream) {
  return this->impl_->upload_file(sp, user_mng, channel_id, name, mimetype, input_stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////
vds::async_task<> vds::file_manager_private::_file_operations::upload_file(
  const service_provider& paren_sp,
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::string& name,
  const std::string& mimetype,
  const filename& file_path) {

  auto sp = paren_sp.create_scope(__FUNCTION__);


  return sp.get<db_model>()->async_transaction(
    sp,
    [pthis = this->shared_from_this(),
      sp,
      user_mng,
      name,
      mimetype,
      file_path,
      channel_id](
    database_transaction& t) {


      auto channel = user_mng->get_channel(sp, channel_id);
      if (!channel.write_cert()) {
        sp.get<logger>()->error(
          ThisModule,
          sp,
          "Channel %s don't have write cert",
          base64::from_bytes(channel_id).c_str());
        throw vds_exceptions::access_denied_error("User don't have write permission");
      }
      auto channel_write_key = user_mng->get_channel_write_key(sp, channel.id());

      std::list<transactions::file_add_transaction::file_block_t> file_blocks;
      pthis->pack_file(sp, file_path, t, file_blocks);

      throw std::runtime_error("Not implemented");
      //transactions::transaction_block log;
      //log.add(
      //  transactions::file_add_transaction(
      //    name,
      //    mimetype,
      //    file_blocks));

      //log.save(
      //  sp,
      //  t,
      //  channel_id,
      //  channel.read_cert(),
      //  channel.write_cert(),
      //  channel_write_key);

      return true;
    }
  );
}

vds::async_task<vds::const_data_buffer> vds::file_manager_private::_file_operations::upload_file(
  const service_provider& paren_sp,
  const std::shared_ptr<user_manager>& user_mng,
  const const_data_buffer& channel_id,
  const std::string& name,
  const std::string& mimetype,
  const std::shared_ptr<continuous_buffer<uint8_t>>& input_stream) {

  auto sp = paren_sp.create_scope(__FUNCTION__);
  return this->pack_file(sp, input_stream)
             .then(
               [sp,
                 pthis = this->shared_from_this(),
                 user_mng,
                 name,
                 mimetype,
                 input_stream,
                 channel_id](const pack_file_result & file_info) {
                 return sp.get<db_model>()->async_transaction(
                   sp,
                   [pthis,
                     sp,
                     user_mng,
                     name,
                     mimetype,
                     input_stream,
                     channel_id,
                     file_info](
                   database_transaction& t) {


                     auto channel = user_mng->get_channel(sp, channel_id);
                     if (!channel.write_cert()) {
                       sp.get<logger>()->error(
                         ThisModule,
                         sp,
                         "Channel %s don't have write cert",
                         base64::from_bytes(channel_id).c_str());
                       throw vds_exceptions::access_denied_error("User don't have write permission");
                     }
                     auto channel_write_key = user_mng->get_channel_write_key(sp, channel.id());

                     transactions::transaction_block log;
                     log.add(
                       transactions::file_add_transaction(
                         file_info.total_hash,
                         file_info.total_size,
                         name,
                         mimetype,
                         file_info.file_blocks));

                     log.save(
                       sp,
                       t,
                       channel_id,
                       channel.read_cert(),
                       channel.write_cert(),
                       channel_write_key);

                     return true;
                   }).then([file_info]() {
                     return file_info.total_hash;
                   });
               });
}

vds::async_task<> vds::file_manager_private::_file_operations::download_file(
  const service_provider& parent_sp,
  const std::shared_ptr<file_manager::download_file_task>& task) {
  auto sp = parent_sp.create_scope(__FUNCTION__);
  return vds::async_task<>([sp, task, pthis = this->shared_from_this()](const async_result<>& result) {
    sp.get<db_model>()->async_transaction(
      sp,
      [pthis, sp, task, result](database_transaction& t) -> bool {
        if (task->file_blocks().empty()) {

          auto user_mng = sp.get<user_manager>();
          auto channel = user_mng->get_channel(sp, task->channel_id());

          user_mng->walk_messages(
            sp,
            task->channel_id(),
            t,
            [task](const transactions::file_add_transaction& message)-> bool {
              if (task->name() == message.name()) {
                task->set_file_blocks(
                  message.mimetype(),
                  message.file_blocks());
                return false;
              }

              return true;
            });
        }

        auto runner = new _async_series(result, task->remote_block_count());
        for (auto& block : task->file_blocks()) {
          if (!block.is_processed_) {
            runner->add(pthis->download_block(sp, t, block, task));
          }
        }

        return true;
      }).execute([result, task](const std::shared_ptr<std::exception>& ex) {
      if (ex) {
        result.error(ex);
      }
      else if (task->mime_type().empty()) {
        result.error(std::make_shared<vds_exceptions::not_found>());
      }
    });
  });
}

vds::async_task<> vds::file_manager_private::_file_operations::download_block(
  const service_provider& sp,
  database_transaction& t,
  file_manager::download_file_task::block_info& block,
  const std::shared_ptr<file_manager::download_file_task>& result) {
  /*
  auto left_replicas(block.id_.replica_hashes);
  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;

  orm::chunk_replica_data_dbo t1;
  auto st = t.get_reader(
      t1
          .select(t1.replica, t1.replica_data)
          .where(t1.id == base64::from_bytes(block.id_.block_id)));

  while (st.execute()) {
    auto replica = safe_cast<uint16_t>(t1.replica.get(st));
    replicas.push_back(replica);

    auto replica_data = t1.replica_data.get(st);
    datas.push_back(replica_data);
    vds_assert(hash::signature(hash::sha256(), replica_data) == left_replicas[replica]);
    left_replicas.erase(replica);

    if (replicas.size() >= chunk_replicator::MIN_HORCRUX) {
      break;
    }
  }

  if (replicas.size() >= chunk_replicator::MIN_HORCRUX) {

    chunk_restore<uint16_t> restore(chunk_replicator::MIN_HORCRUX, replicas.data());
    binary_serializer s;
    restore.restore(s, datas);

    auto data = chunk_manager::unpack_block(
        block.id_.block_id,
        block.id_.block_key,
        s.data().data(),
        s.size() - block.id_.padding);

    result->set_file_data(block, data);
  } else {
    auto p2p = sp.get<p2p_network>();
    auto user_mng = sp.get<user_manager>();

    asymmetric_private_key device_private_key;
    auto device_user = user_mng->get_current_device(sp, device_private_key);

    for(auto &  replica : left_replicas) {

      sp.get<logger>()->warning(
          ThisModule,
          sp,
          "Send query for replica %s",
          std::to_string(replica.first).c_str());

      //p2p->query_replica(sp, replica.second);
    }
  }
  */
  return async_task<>::empty();
}

void vds::file_manager_private::_file_operations::pack_file(
  const service_provider& sp,
  const filename& file_path,
  database_transaction& t,
  std::list<transactions::file_add_transaction::file_block_t>& file_blocks) const {
  /*
  auto chunk_mng = sp.get<vds::chunk_manager>();
  auto file_size = vds::file::length(file_path);
  if (file_size > 0) {
    if (file_size < vds::file_manager::file_operations::BLOCK_SIZE) {
		resizable_data_buffer buffer(file_size);
      vds::file f(file_path, vds::file::file_mode::open_read);

      auto readed = f.read(buffer.data(), file_size);
      vds_assert(readed == file_size);
      size_t padding;
      std::unordered_map<uint16_t, const_data_buffer> replica_hashes;
      auto block_info = chunk_mng->save_block(
        sp,
          t, vds::const_data_buffer(buffer.data(), readed),
          padding,
          replica_hashes);
      file_blocks.push_back(
          transactions::file_add_transaction::file_block_t
          {
              block_info.id,
              block_info.key,
              safe_cast<decltype(transactions::file_add_transaction::file_block_t::block_size)>(readed),
              static_cast<uint16_t>(padding),
              replica_hashes
          });
    } else {
      auto count = file_size / vds::file_manager::file_operations::BLOCK_SIZE;
      auto block_size = (file_size + count - 1) / count;
		resizable_data_buffer buffer(block_size);
      vds::file f(file_path, vds::file::file_mode::open_read);

      for (uint64_t offset = 0; offset < file_size; offset += block_size) {
        auto readed = f.read(buffer.data(), block_size);
        size_t padding;
        std::unordered_map<uint16_t, const_data_buffer> replica_hashes;
        auto block_info = chunk_mng->save_block(
          sp,
            t, vds::const_data_buffer(buffer.data(), readed),
            padding, replica_hashes);
        file_blocks.push_back(
            transactions::file_add_transaction::file_block_t
                {
                    block_info.id,
                    block_info.key,
                    safe_cast<decltype(transactions::file_add_transaction::file_block_t::block_size)>(readed),
                    static_cast<uint16_t>(padding),
                    replica_hashes
                });
      }
    }
  }
  */
}

struct buffer_data : public std::enable_shared_from_this<buffer_data> {
  uint8_t buffer[vds::file_manager::file_operations::BLOCK_SIZE];
};

vds::async_task<vds::file_manager_private::_file_operations::pack_file_result>
vds::file_manager_private::_file_operations::pack_file(
  const service_provider& sp,
  const std::shared_ptr<continuous_buffer<uint8_t>>& input_stream) const {
  auto task = std::make_shared<_upload_stream_task>();
  return task->start(sp, input_stream).then([task](const std::list<transactions::file_add_transaction::file_block_t> & file_blocks) {
    return pack_file_result{ task->result_hash(), task->total_size(), file_blocks };
  });
}

void
vds::file_manager_private::_file_operations::restore_chunk(
  const service_provider& sp,
  database_transaction& t,
  file_manager::download_file_task::block_info& block,
  const std::shared_ptr<file_manager::download_file_task>& result) {
  /*
  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;

  orm::chunk_replica_data_dbo t1;
  auto st = t.get_reader(
      t1
          .select(t1.replica, t1.replica_data)
          .where(t1.id == base64::from_bytes(block.id_.block_id)));

  while(st.execute()){
    replicas.push_back(safe_cast<uint16_t>(t1.replica.get(st)));
    datas.push_back(t1.replica_data.get(st));

    if(replicas.size() >= chunk_replicator::MIN_HORCRUX){
      break;
    }
  }

  if(replicas.size() < chunk_replicator::MIN_HORCRUX){
    throw std::runtime_error("Invalid operation");
  }

  chunk_restore<uint16_t> restore(chunk_replicator::MIN_HORCRUX, replicas.data());
  binary_serializer s;
  restore.restore(s, datas);

//  dbo::chunk_data_dbo t2;
//  t.execute(t2.insert(
//          t2.id = base64::from_bytes(block.id_.block_id),
//      t2.block_key = block.id_.block_key));

  result->set_file_data(block, s.data());
  */
}
