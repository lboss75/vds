
#include "stdafx.h"
#include <set>
#include <transaction_block.h>
#include <transactions/channel_add_message_transaction.h>
#include <chunk.h>
#include <chunk_replica_data_dbo.h>
#include "chunk_manager.h"
#include "chunk_manager.h"
#include "private/chunk_manager_p.h"
#include "deflate.h"
#include "inflate.h"
#include "database_orm.h"
#include "chunk_data_dbo.h"
#include "chunk_replicator.h"
#include "vds_debug.h"
#include "user_manager.h"
#include "run_configuration_dbo.h"

static uint8_t pack_block_iv[] = {
    // 0     1     2     3     4     5     6     7
    0xa5, 0xbb, 0x9f, 0xce, 0xc2, 0xe4, 0x4b, 0x91,
    0xa8, 0xc9, 0x59, 0x44, 0x62, 0x55, 0x90, 0x24
};

vds::chunk_manager::chunk_info vds::chunk_manager::pack_block(
    const vds::const_data_buffer &data) {

  auto key_data = hash::signature(hash::sha256(), data);

  if(key_data.size() != symmetric_crypto::aes_256_cbc().key_size()
     || sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()){
    throw std::runtime_error("Design error");
  }

  auto key = symmetric_key::create(
      symmetric_crypto::aes_256_cbc(),
      key_data.data(),
      pack_block_iv);

  auto key_data2 = hash::signature(
      hash::sha256(),
      symmetric_encrypt::encrypt(key, data));

  auto key2 = symmetric_key::create(
      symmetric_crypto::aes_256_cbc(),
      key_data2.data(),
      pack_block_iv);

  auto zipped = deflate::compress(data);

  return chunk_info
  {
      key_data,
      key_data2,
      symmetric_encrypt::encrypt(key2, zipped)
  };
}

vds::const_data_buffer vds::chunk_manager::unpack_block(
    const const_data_buffer & block_id,
    const const_data_buffer & block_key,
    const void * block_data,
    size_t block_size)
{
	if (block_key.size() != symmetric_crypto::aes_256_cbc().key_size()
		|| sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()) {
		throw std::runtime_error("Design error");
	}

	auto key2 = symmetric_key::create(
		symmetric_crypto::aes_256_cbc(),
		block_key.data(),
		pack_block_iv);

	auto zipped = symmetric_decrypt::decrypt(key2, block_data, block_size);
	auto data = inflate::decompress(zipped.data(), zipped.size());
	if (block_id != hash::signature(hash::sha256(), data)) {
		throw std::runtime_error("Data block is corrupt");
	}

	return data;
}


vds::chunk_manager::chunk_info
vds::chunk_manager::save_block(
    const service_provider & sp,
		vds::database_transaction &t,
		const vds::const_data_buffer &data,
    size_t & padding,
		std::unordered_map<uint16_t, const_data_buffer> & replica_hashes) {
  
	auto block = pack_block(data);
  /*
//	orm::chunk_data_dbo t1;
//	t.execute(t1.insert(
//			t1.id = base64::from_bytes(block.id),
//			t1.block_key = block.key));

  resizable_data_buffer padded_data;
  padded_data += block.data;
  if (0 != (padded_data.size() % (sizeof(uint16_t) * chunk_replicator::MIN_HORCRUX))) {
    padding = sizeof(uint16_t) * chunk_replicator::MIN_HORCRUX - (padded_data.size() % (sizeof(uint16_t) * chunk_replicator::MIN_HORCRUX));
    padded_data.padding(padding);
  }
  else {
    padding = 0;
  }

	for(int16_t replica = 0; replica < chunk_replicator::GENERATE_HORCRUX; ++replica){
		chunk_generator<uint16_t> generator(chunk_replicator::MIN_HORCRUX, replica);

		binary_serializer s;
		generator.write(s, padded_data.data(), padded_data.size());

		auto replica_hash = hash::signature(hash::sha256(), s.data());
		replica_hashes[replica] = replica_hash;

    sp.get<logger>()->trace(ThisModule, sp, "Save replica %d of %s", replica, base64::from_bytes(block.id).c_str());

		orm::chunk_replica_data_dbo t2;
		t.execute(t2.insert(
			t2.id = base64::from_bytes(block.id),
			t2.replica = (int)replica,
			t2.replica_data = s.data(),
			t2.replica_hash = replica_hash));
	}
  */
	return block;
}

