/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

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
#include "transactions/channel_add_message_transaction.h"
#include "vds_exceptions.h"
#include "cert_control.h"
#include "chunk_data_dbo.h"
#include "chunk_replica_data_dbo.h"
#include "chunk_replicator.h"
#include "chunk.h"
#include "p2p_network.h"
#include "messages/chunk_query_replica.h"

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

vds::async_task<>
vds::file_manager::file_operations::download_file(
    const service_provider &sp,
    const std::shared_ptr<download_file_task> & task)
{
	return this->impl_->download_file(sp, task);
}

//////////////////////////////////////////////////////////////////////////////////////////////
vds::async_task<> vds::file_manager_private::_file_operations::upload_file(
    const service_provider & paren_sp,
    const guid & channel_id,
    const std::string &name,
    const std::string &mimetype,
    const vds::filename &file_path) {

	auto sp = paren_sp.create_scope(__FUNCTION__);


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
        auto user = user_mng->get_current_device(sp, device_private_key);

        auto channel = user_mng->get_channel(channel_id);
        if(!channel.write_cert()){
          sp.get<logger>()->error(
              ThisModule,
              sp,
              "Channel %s don't have write cert",
              channel_id.str().c_str());
          throw vds_exceptions::access_denied_error("User don't have write permission");
        }
        auto channel_write_key = user_mng->get_channel_write_key(channel.id());

        std::list<transactions::file_add_transaction::file_block_t> file_blocks;
        pthis->pack_file(sp, file_path, t, file_blocks);

        transactions::transaction_block log;
        log.add(
            transactions::file_add_transaction(
                channel_id,
                channel.read_cert(),
                cert_control::get_id(channel.write_cert()),
                channel_write_key,
                name,
                mimetype,
                file_blocks));

		auto common_channel_write_key = user_mng->get_channel_write_key(user_mng->get_common_channel().id());
		log.save(
            sp,
            t,
			user_mng->get_common_channel().read_cert(),
			user_mng->get_common_channel().write_cert(),
			common_channel_write_key);

		return true;
      }
  );
}

vds::async_task<> vds::file_manager_private::_file_operations::download_file(
	const service_provider& parent_sp,
  const std::shared_ptr<vds::file_manager::download_file_task> & task)
{
	auto sp = parent_sp.create_scope(__FUNCTION__);
	return vds::async_task<>([sp, task, pthis = this->shared_from_this()](const vds::async_result<> & result) {
		sp.get<db_model>()->async_transaction(
			sp,
			[pthis, sp, task, result](database_transaction & t) {
        if(task->file_blocks().empty()) {

          auto user_mng = sp.get<user_manager>();
          auto channel = user_mng->get_channel(task->channel_id());

          dbo::channel_message t1;
          auto st = t.get_reader(
              t1
                  .select(
                      t1.id,
                      t1.message_id,
                      t1.message,
                      t1.read_cert_id,
                      t1.write_cert_id,
                      t1.signature)
                  .where(t1.channel_id == task->channel_id()
                         && t1.message_id ==
                            (int) transactions::channel_message_transaction::channel_message_id::file_add_transaction)
                  .order_by(db_desc_order(t1.id)));
          while (st.execute()) {
            auto data = user_mng->decrypt_message(
                sp,
                task->channel_id(),
                t1.message_id.get(st),
                t1.read_cert_id.get(st),
                t1.write_cert_id.get(st),
                t1.message.get(st),
                t1.signature.get(st));

            binary_deserializer s(data);

            transactions::file_add_transaction::parse_message(s, [pthis, sp, &t, result, task](
                const std::string &record_name,
                const std::string &record_mimetype,
                const std::list<transactions::file_add_transaction::file_block_t> &file_blocks) {
              if (task->name() == record_name) {
                task->set_file_blocks(record_mimetype, file_blocks);
              }
            });

            if (!task->mime_type().empty()) {
              break;
            }
          }
        }

        auto runner = new _async_series(result, task->remote_block_count());
        for (auto &block : task->file_blocks()) {
          if(!block.is_processed_) {
            runner->add(pthis->download_block(sp, t, block, task));
          }
        }

			return true;
		}).execute([result, task](const std::shared_ptr<std::exception> & ex) {
			if(ex) {
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
  file_manager::download_file_task::block_info & block,
  const std::shared_ptr<file_manager::download_file_task> & result) {
  auto left_replicas(block.id_.replica_hashes);
  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;

  dbo::chunk_replica_data_dbo t1;
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

      p2p->query_replica(sp, replica.second);
    }
  }
  return async_task<>::empty();
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
		resizable_data_buffer buffer(file_size);
      vds::file f(file_path, vds::file::file_mode::open_read);

      auto readed = f.read(buffer.data(), file_size);
      vds_assert(readed == file_size);
      size_t padding;
      std::unordered_map<uint16_t, const_data_buffer> replica_hashes;
      auto block_info = chunk_mng->save_block(
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
}

void
  vds::file_manager_private::_file_operations::restore_chunk(
      const vds::service_provider &sp,
      vds::database_transaction &t,
      vds::file_manager::download_file_task::block_info &block,
      const std::shared_ptr<vds::file_manager::download_file_task> &result) {

  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;

  dbo::chunk_replica_data_dbo t1;
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

  dbo::chunk_data_dbo t2;
  t.execute(t2.insert(
          t2.id = base64::from_bytes(block.id_.block_id),
      t2.block_key = block.id_.block_key));

  result->set_file_data(block, s.data());
}

