
#include "stdafx.h"
#include <set>
#include <transaction_block.h>
#include <transactions/channel_add_message_transaction.h>
#include "chunk_manager.h"
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
      .id = key_data,
      .key = key_data2,
      .data = symmetric_encrypt::encrypt(key2, zipped)
  };
}

vds::const_data_buffer vds::chunk_manager::unpack_block(
    const const_data_buffer & block_id,
    const const_data_buffer & block_key,
    const const_data_buffer & block_data)
{
	if (block_key.size() != symmetric_crypto::aes_256_cbc().key_size()
		|| sizeof(pack_block_iv) != symmetric_crypto::aes_256_cbc().iv_size()) {
		throw std::runtime_error("Design error");
	}

	auto key2 = symmetric_key::create(
		symmetric_crypto::aes_256_cbc(),
		block_key.data(),
		pack_block_iv);

	auto zipped = symmetric_decrypt::decrypt(key2, block_data);
	auto data = inflate::decompress(zipped.data(), zipped.size());
	if (block_id != hash::signature(hash::sha256(), data)) {
		throw std::runtime_error("Data block is corrupt");
	}

	return data;
}
