/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <compare_data.h>
#include "stdafx.h"
#include "test_vds.h"
#include "vds_mock.h"

TEST(test_vds, test_initial)
{
  try{
    vds_mock mock;

    mock.start(5);

    //Waiting to sync logs
    mock.sync_wait();

    auto channel = mock.create_channel(0, "Test file");

    size_t len;
    do
    {
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 32 * 1024 * 1024;
    } while (len < 1024 || len > 32 * 1024 * 1024);

    std::unique_ptr<uint8_t> buffer(new uint8_t[len]);
    vds::crypto_service::rand_bytes(buffer.get(), (int)len);

    //Waiting to sync logs
    mock.sync_wait();

    mock.allow_write_channel(3, channel.id());

    vds::barrier b;
    std::shared_ptr<std::exception> error;
    auto sp = mock.get_sp(3);
    vds::mt_service::enable_async(sp);
    auto input_stream = std::make_shared<vds::continuous_buffer<uint8_t>>(sp);
    input_stream->write_async(buffer.get(), len).then([input_stream](){
      return input_stream->write_async(nullptr, 0);
    }).execute([&b, &error](const std::shared_ptr<std::exception> & ex){
      if(ex){
        error = ex;
      }
      b.set();
    });

    std::cout << "Upload local file...\n";
    auto file_hash = mock.upload_file(3, channel.id(), "test data", "application/octet-stream", input_stream);
    b.wait();
    ASSERT_TRUE(!error);

    std::cout << "Download local file...\n";

    std::string result_mime;
    size_t result_size;
    std::shared_ptr<vds::stream<uint8_t>> result_data = std::make_shared<compare_data<uint8_t>>(buffer.get(), len);

    mock.download_data(3, channel.id(), "test data", file_hash)
        .then([sp, &result_mime, &result_size, &result_data](
                    const std::string & content_type,
                    size_t body_size,
                    const std::shared_ptr<vds::continuous_buffer<uint8_t>> & output_stream){
            result_mime = content_type;
            result_size = body_size;
            return vds::copy_stream(sp, output_stream, result_data);
        }).wait();

    ASSERT_EQ(len, result_size);

    mock.allow_read_channel(4, channel.id());
    //Waiting to sync logs
    mock.sync_wait();
    std::cout << "Download file...\n";

//	mock.download_data(4, channel.id(), "test data", tmp_ofn);
//	auto result = vds::file::read_all(tmp_ofn);

    mock.stop();
   
//    ASSERT_EQ(len, result.size());
//    ASSERT_EQ(memcmp(buffer.get(), result.data(), len), 0);
  }
  catch (const std::exception & ex) {
    GTEST_FAIL() << ex.what();
  }
  catch(...){
    GTEST_FAIL() << "Unexpected error";
  }
}