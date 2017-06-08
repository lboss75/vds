#include "file_manager.h"
#include "file_manager_p.h"
#include "file.h"
#include "symmetriccrypto.h"

////////////////////////////////////////
/*
vds::async_task<> vds::upload_file(
  const service_provider & sp,
  const std::string & name,
  const filename & fn)
{
  file_object result;
  file_header header;
  constexpr size_t BLOCK_SIZE = 5 * 1024 * 1024;
  
  auto lenght = file::length(fn);
  for(decltype(lenght) start = 0; start < lenght; start += BLOCK_SIZE){
    size_t original_lenght;
    const_data_buffer original_hash;
    size_t target_lenght;
    const_data_buffer target_hash;
    dataflow(
      file_range_read(fn, start, BLOCK_SIZE),
      hash_filter(&original_lenght, &original_hash),
      deflate(),
      symmetric_encrypt(),      
      hash_filter(&target_lenght, &target_hash),
      file_write(tmp_file, file::file_mode::append)
    )(
      [this, tmp_file, &original_lenght, &original_hash, &target_lenght, &target_hash](const service_provider & sp) {
        header << original_lenght << original_hash << target_lenght << target_hash;
      },
      [](const service_provider & sp, std::exception_ptr ex) {
        
      },
      sp);
  }
  
  file_channel = this->decrypt_file_channel();
  
  file_channel->add_record(upload_file());
  
  this->client_->push_object();
}
*/