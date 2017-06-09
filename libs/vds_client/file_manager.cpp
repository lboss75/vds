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

////////////////////////////////////////

vds::async_task<> vds::_file_manager::upload_file(
  const service_provider & sp,
  const certificate & user_certificate,
  const std::string & name,
  const filename & fn)
{
  return create_async_task(
    [this, user_certificate, name, fn](const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp) {
        imt_service::disable_async(sp);


        //Generate key
        symmetric_key transaction_key(symmetric_crypto::aes_256_cbc());
        transaction_key.generate();

        //Meta info
        binary_serializer file_info;
        auto lenght = file::length(fn);
        file_info << name << lenght;
        transaction_key.serialize(file_info);
        auto meta_info = user_certificate.public_key().encrypt(file_info.data());

        //Crypt body
        auto body_id = guid::new_guid();

        sp.get<logger>()->trace(sp, "Crypting data");

        constexpr size_t BLOCK_SIZE = 5 * 1024 * 1024;
        binary_serializer header;
        filename tmp_file(this->tmp_folder_, body_id.str());

        for (decltype(lenght) start = 0; start < lenght; start += BLOCK_SIZE) {
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
            [this, &header, &original_lenght, &original_hash, &target_lenght, &target_hash](const service_provider & sp) {
              header << original_lenght << original_hash << target_lenght << target_hash;
            },
            [tmp_file, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              file::delete_file(tmp_file);
              on_error(sp, ex);
            },
            sp);
        }

        //Crypt header
        auto header_id = guid::new_guid();
        size_t header_lenght;
        const_data_buffer header_hash;
        dataflow(
          dataflow_arguments<uint8_t>(header.data().data(), header.size()),
          deflate(),
          symmetric_encrypt(transaction_key),
          hash_filter(&header_lenght, &header_hash),
          file_write(tmp_file, file::file_mode::create_new)
        )(
          [this](const service_provider & sp) {
          },
          [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          },
          sp);

  //
  binary_serializer file_info;
  file_info << name << header_id;

  std::vector<uint8_t> file_info_crypted;
  dataflow(
    dataflow_arguments<uint8_t>(file_info.data().data(), file_info.size()),
    symmetric_encrypt(transaction_key),
    collect_data(file_info_crypted)
  )(
    [](const service_provider & sp) {
    },
    [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    },
    sp);



  auto key_crypted = user_certificate.public_key().encrypt(key_data.data());

  binary_serializer to_sign;
  to_sign << file_info_crypted << key_crypted;

  binary_serializer datagram;
  datagram << key_crypted;
  datagram << asymmetric_sign::signature(
    hash::sha256(),
    user_private_key,
    to_sign.data());

  dataflow(
    file_read(fn),
    deflate(),
    symmetric_encrypt(transaction_key),
    file_write(tmp_file, file::file_mode::create_new))(
      [this, tmp_file, user_login, name, done, on_error, info = const_data_buffer(datagram.data())](const service_provider & sp) {

    sp.get<logger>()->trace(sp, "Upload file");
    this->owner_->logic_->put_file(
      sp,
      user_login,
      name,
      info,
      tmp_file)
      .wait(done, on_error, sp);
  },
      [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) { std::rethrow_exception(ex); },
    sp);



  file_object result;
  file_header header;
  
  file_channel = this->decrypt_file_channel();
  
  file_channel->add_record(upload_file());
  
  this->client_->push_object();
}

vds::async_task<> vds::_file_manager::download_file(
  const service_provider & sp,
  const certificate & user_certificate,
  const std::string & name,
  const filename & fn)
{
  principal_log_objects log_record;
  log_record.meta_info() >> file_info_crypted;

  file_info_crypted >> file_name;

  const_data_buffer file_info_crypted;
  const_data_buffer key_crypted;

  binary_deserializer signed_block;
  signed_block >> file_info_crypted >> key_crypted;

}