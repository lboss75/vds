#include "file_manager.h"
#include "file_manager_p.h"
#include "file.h"
#include "symmetriccrypto.h"
#include "hash.h"
#include "deflate.h"
#include "logger.h"
#include "guid.h"
#include "mt_service.h"
#include "asymmetriccrypto.h"
#include "log_records.h"
#include "foldername.h"
#include "json_object.h"

////////////////////////////////////////
vds::async_task<size_t /*body_size*/, size_t /*tail_size*/> vds::_file_manager::crypt_file(
  const service_provider & sp,
  size_t length,
  const symmetric_key & transaction_key,
  const filename & fn,
  const filename & tmp_file)
{
  return create_async_task(
    [length, transaction_key, fn, tmp_file]
    (const std::function<void(const service_provider & sp, size_t body_size, size_t tail_size)> & done,
      const error_handler & on_error,
      const service_provider & sp) {

      constexpr size_t BLOCK_SIZE = 5 * 1024 * 1024;
      
      size_t total_size = 0;
      binary_serializer tail;

      for (decltype(length) start = 0; start < length; start += BLOCK_SIZE) {
        size_t original_lenght;
        const_data_buffer original_hash;
        size_t target_lenght;
        const_data_buffer target_hash;
        dataflow(
          file_range_read(fn, start, BLOCK_SIZE),
          hash_filter(&original_lenght, &original_hash),
          deflate(),
          symmetric_encrypt(transaction_key),
          hash_filter(&target_lenght, &target_hash),
          file_write(tmp_file, file::file_mode::append)
        )(
          [&tail, &original_lenght, &original_hash, &target_lenght, &target_hash, &total_size]
          (const service_provider & sp) {
            total_size += target_lenght;
            tail << original_lenght << original_hash << target_lenght << target_hash;
          },
          [tmp_file, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            file::delete_file(tmp_file);
            on_error(sp, ex);
          },
          sp);
      }
      
      size_t tail_lenght;
      const_data_buffer tail_hash;
      dataflow(
        dataflow_arguments<uint8_t>(tail.data().data(), tail.size()),
        deflate(),
        symmetric_encrypt(transaction_key),
        hash_filter(&tail_lenght, &tail_hash),
        file_write(tmp_file, file::file_mode::append)
      )(
        [done, total_size, &tail_lenght](const service_provider & sp) {
          done(sp, total_size, tail_lenght);
        },
        [tmp_file, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          file::delete_file(tmp_file);
          on_error(sp, ex);
        },
        sp);
      
    });
}