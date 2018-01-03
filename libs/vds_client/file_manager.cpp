#include "file_manager_service.h"
#include "file.h"
#include "symmetriccrypto.h"
#include "hash.h"
#include "deflate.h"
#include "inflate.h"
#include "logger.h"
#include "guid.h"
#include "mt_service.h"
#include "asymmetriccrypto.h"
#include "log_records.h"
#include "foldername.h"
#include "json_object.h"
#include "private/file_manager_p.h"

////////////////////////////////////////
vds::async_task<size_t /*body_size*/, size_t /*tail_size*/> vds::_file_manager::crypt_file(
  const service_provider & sp,
  size_t length,
  const symmetric_key & transaction_key,
  const filename & fn,
  const filename & tmp_file)
{
  return [sp, length, transaction_key, fn, tmp_file]
    (const async_result<size_t /*body_size*/, size_t /*tail_size*/> & result) {

      constexpr size_t BLOCK_SIZE = 5 * 1024 * 1024;
      
      size_t total_size = 0;
      binary_serializer tail;

      file f(fn, file::file_mode::open_read);
      file fout(tmp_file, file::file_mode::append);
      
      uint8_t buffer[BLOCK_SIZE];
      for (decltype(length) start = 0; start < length; start += BLOCK_SIZE) {
        
        auto original_lenght = f.read(buffer, sizeof(buffer));
        if(0 == original_lenght){
          break;
        }
        
        auto original_hash = hash::signature(hash::sha256(), buffer, original_lenght);
        auto data = deflate::compress(
          symmetric_encrypt::encrypt(transaction_key, buffer, original_lenght));
        auto target_hash = hash::signature(hash::sha256(), data);
        
        fout.write(data);
        
        total_size += data.size();
        tail << original_lenght << original_hash << data.size() << target_hash;
      }
      
      result.done(total_size, 0);
    };
}

vds::async_task<> vds::_file_manager::decrypt_file(
  const service_provider & sp,
  const symmetric_key & transaction_key,
  const filename & tmp_file,
  const filename & target_file,
  size_t body_size,
  size_t tail_size)
{
  return [](){};
//   auto tail_data = std::make_shared<std::vector<uint8_t>>();
//   
//   return dataflow(
//         file_range_read(tmp_file, body_size, tail_size),
//         symmetric_decrypt(transaction_key),
//         inflate(),
//         collect_data(*tail_data))
//   .then(
//       [tail_data, tmp_file, target_file, transaction_key](
//         const std::function<void (const service_provider & sp)> & done,
//         const error_handler & on_error,
//         const service_provider & sp){
//             
//             binary_deserializer tail(*tail_data);
//             size_t offset = 0;
//             while(0 < tail.size()) {
//               size_t original_lenght;
//               const_data_buffer original_hash;
//               size_t target_lenght;
//               const_data_buffer target_hash;
//               
//               tail >> original_lenght >> original_hash >> target_lenght >> target_hash;
//               
//               std::shared_ptr<std::exception> error;
//               dataflow(
//                 file_range_read(tmp_file, offset, target_lenght),
//                 symmetric_decrypt(transaction_key),
//                 inflate(),
//                 file_write(target_file, (0 == offset) ? file::file_mode::truncate : file::file_mode::append)
//               ).wait(
//                 [](const service_provider & sp){
//                 },
//                 [&error](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
//                   error = ex;
//                 },
//                 sp);
//               
//               offset += target_lenght;
//               if(error) {
//                 on_error(sp, error);
//                 return;
//               }
//             }
//             
//             done(sp);
//   });
}

