/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <compare_data.h>
#include "stdafx.h"
#include "test_vds.h"
#include "vds_mock.h"
#include "test_config.h"

TEST(test_vds, test_initial)
{
  try{
    vds_mock mock;

    mock.start(16);

    //Waiting to sync logs
    mock.sync_wait();

    std::cout << "Create channel...\n";
    auto channel = mock.create_channel(0, "test", "Test file");

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

    mock.get_sp(3);

    std::cout << "Upload local file...\n";
    auto file_hash = mock.upload_file(
      3,
      channel.id(),
      "test data",
      "application/octet-stream",
      std::make_shared<vds::buffer_stream_input_async>(vds::const_data_buffer(buffer.get(), len)));

    std::cout << "Download local file...\n";

    auto result_data = std::make_shared<compare_data_async<uint8_t>>(buffer.get(), len);

    GET_EXPECTED_GTEST(result, mock.download_data(3, channel.id(), "test data", file_hash, result_data).get());

    ASSERT_EQ(len, result.size);

    mock.allow_read_channel(4, channel.id());
    //Waiting to sync logs
    mock.sync_wait();
    std::cout << "Download file...\n";

    result_data = std::make_shared<compare_data_async<uint8_t>>(buffer.get(), len);
    GET_EXPECTED_VALUE_GTEST(result, mock.download_data(4, channel.id(), "test data", file_hash, result_data).get());

    std::cout << "Done...\n";
    mock.stop();

    ASSERT_EQ(len, result.size);

  }
  catch (const std::exception & ex) {
    GTEST_FAIL() << ex.what();
  }
  catch(...){
    GTEST_FAIL() << "Unexpected error";
  }
}