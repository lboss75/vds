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
#include "chunk_replica_dbo.h"
#include "chunk_replicator.h"

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

vds::async_task<vds::file_manager::file_operations::download_file_result_t>
vds::file_manager::file_operations::download_file(const service_provider& sp,
	const vds::guid& channel_id, const std::string& name, const vds::filename& file_path)
{
	return this->impl_->download_file(sp, channel_id, name, file_path);
}

//////////////////////////////////////////////////////////////////////////////////////////////
vds::async_task<> vds::file_manager_private::_file_operations::upload_file(
    const service_provider & paren_sp,
    const guid & channel_id,
    const std::string &name,
    const std::string &mimetype,
    const vds::filename &file_path) {

	auto sp = paren_sp.create_scope(__FUNCSIG__);


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

vds::async_task<vds::file_manager::file_operations::download_file_result_t> vds::file_manager_private::_file_operations::download_file(
	const service_provider& parent_sp,
	const guid& channel_id,
	const std::string& name,
	const filename& file_path)
{
	auto sp = parent_sp.create_scope(__FUNCSIG__);
	auto result_data = std::make_shared<vds::file_manager::file_operations::download_file_result_t>();
	return vds::async_task<>([sp, name, channel_id, file_path, result_data, pthis = this->shared_from_this()](const vds::async_result<> & result) {
		sp.get<db_model>()->async_transaction(
			sp,
			[pthis, sp, name, result, file_path, result_data, channel_id](database_transaction & t) {

			auto user_mng = sp.get<user_manager>();
			auto channel = user_mng->get_channel(channel_id);

			dbo::channel_message t1;
			auto st = t.get_reader(
				t1
				.select(t1.id, t1.message)
				.where(t1.channel_id == channel_id
					&& t1.message_id == (int)transactions::channel_message_transaction::channel_message_id::file_add_transaction)
				.order_by(db_desc_order(t1.id)));
			while (st.execute())
			{
				auto data = t1.message.get(st);

				binary_deserializer s(data);

				transactions::file_add_transaction::parse_message(s, [pthis, sp, &t, result, result_data, name](
					const std::string &record_name,
					const std::string &record_mimetype,
					const std::list<transactions::file_add_transaction::file_block_t> &file_blocks) {
					if (name == record_name) {
						result_data->mime_type = record_mimetype;
						auto runner = new _async_series(result, file_blocks.size());
						for(auto & block : file_blocks) {
							runner->add(pthis->download_block(sp, t, block, result_data));
						}
					}
				});

				if (!result_data->mime_type.empty()) {
					break;
				}
			}

			return true;
		}).execute([result, result_data](const std::shared_ptr<std::exception> & ex) {
			if(ex) {
				result.error(ex);
			}
			else if (result_data->mime_type.empty()) {
				result.error(std::make_shared<vds_exceptions::not_found>());
			}

		});
	}).then([result_data]() {
		return *result_data;
	});
}

vds::async_task<> vds::file_manager_private::_file_operations::download_block(
	const service_provider& sp,
	database_transaction& t,
	const transactions::file_add_transaction::file_block_t& block_id,
	const std::shared_ptr<vds::file_manager::file_operations::download_file_result_t> & result)
{
	dbo::chunk_data_dbo t1;
	auto st = t.get_reader(t1.select(t1.id).where(t1.id == base64::from_bytes(block_id.block_id)));
	if(st.execute())
	{
		result->local_block_count++;
	}
	else
	{
		dbo::chunk_replica_dbo t2;
		st = t.get_reader(t2.select(db_count(t2.id)).where(t2.id == base64::from_bytes(block_id.block_id)));
		st.execute();

		uint64_t count;
		if(!st.get_value(0, count)) {
			throw std::runtime_error("Unexpected error");
		}

		if(count >= chunk_replicator::MIN_HORCRUX)
		{
			result->local_block_count++;
		}
		else
		{
			result->remote_block_count++;

			throw std::runtime_error("Not implemented");
		}
	}
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
      auto block_info = chunk_mng->save_block(t, vds::const_data_buffer(buffer.data(), readed));
      file_blocks.push_back(
          transactions::file_add_transaction::file_block_t
          {
              block_info.id,
              block_info.key
          });
    } else {
      auto count = file_size / vds::file_manager::file_operations::BLOCK_SIZE;
      auto block_size = (file_size + count - 1) / count;
		resizable_data_buffer buffer(block_size);
      vds::file f(file_path, vds::file::file_mode::open_read);

      for (uint64_t offset = 0; offset < file_size; offset += block_size) {
        auto readed = f.read(buffer.data(), block_size);
        auto block_info = chunk_mng->save_block(t, vds::const_data_buffer(buffer.data(), readed));
        file_blocks.push_back(
            transactions::file_add_transaction::file_block_t
                {
                    block_info.id,
                    block_info.key
                });
      }
    }
  }
}

