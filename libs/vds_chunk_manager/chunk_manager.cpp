#include "chunk_manager.h"
#include "stdafx.h"
#include "chunk_manager.h"
#include "private/chunk_manager_p.h"
#include "deflate.h"
#include "inflate.h"
#include "database_orm.h"
#include "chunk_data_dbo.h"

static uint8_t pack_block_iv[] = {
    // 0     1     2     3     4     5     6     7
    0xa5, 0xbb, 0x9f, 0xce, 0xc2, 0xe4, 0x4b, 0x91,
    0xa8, 0xc9, 0x59, 0x44, 0x62, 0x55, 0x90, 0x24
};

vds::chunk_manager::block_id_t vds::chunk_manager::pack_block(
    database_transaction & t,
    const vds::const_data_buffer &data) {
  auto zipped = deflate::compress(data);
  auto key_data = hash::signature(hash::sha256(), zipped);

  if(key_data.size() != symmetric_crypto::aes_256_cbc().key_size()
      || sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()){
    throw std::runtime_error("Design error");
  }

  auto key = symmetric_key::create(
      symmetric_crypto::aes_256_cbc(),
      key_data.data(),
      pack_block_iv);

  auto id = hash::signature(hash::sha256(), data);
  chunk_data_dbo t1;
  t.execute(t1.insert(
      t1.id = id,
      t1.block_key = key_data,
      t1.padding = zipped.size() % key.block_size(),
      t1.block_data = symmetric_encrypt::encrypt(key, zipped)));

  return id;
}

vds::const_data_buffer vds::chunk_manager::get_block(database_transaction & t, const block_id_t & block_id)
{
	chunk_data_dbo t1;
	auto st = t.get_reader(
		t1.select(
			t1.block_key,
			t1.padding,
			t1.block_data)
		.where(t1.id == block_id));
	
	if (!st.execute()) {
		return const_data_buffer();
	}

	auto key_data = t1.block_key.get(st);

	if (key_data.size() != symmetric_crypto::aes_256_cbc().key_size()
		|| sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()) {
		throw std::runtime_error("Design error");
	}

	auto key = symmetric_key::create(
		symmetric_crypto::aes_256_cbc(),
		key_data.data(),
		pack_block_iv);

	auto zipped = symmetric_decrypt::decrypt(key, t1.block_data.get(st));
	auto data = inflate::decompress(zipped.data(), zipped.size() - t1.padding.get(st));
	if (block_id != hash::signature(hash::sha256(), data)) {
		throw std::runtime_error("Data block is corrupt");
	}

	return data;
}
