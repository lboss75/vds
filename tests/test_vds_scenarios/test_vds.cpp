/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
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

    auto folder = vds::foldername(
        vds::foldername(
            vds::filename::current_process().contains_folder(),
            "servers"),
        "tmp");
    folder.delete_folder(true);
    folder.create();

    vds::filename tmp_fn(folder, "data");
    vds::file f(tmp_fn, vds::file::file_mode::create_new);
    f.write(buffer.get(), len);
    f.close();

    //Waiting to sync logs
    mock.sync_wait();

    mock.allow_write_channel(3, channel.id());
    mock.upload_file(3, channel.id(), "test data", "application/octet-stream", tmp_fn);

    std::cout << "Download local file...\n";

	vds::filename tmp_ofn(folder, "out_data");
	mock.download_data(3, channel.id(), "test data", tmp_ofn);

	auto result1 = vds::file::read_all(tmp_ofn);
    ASSERT_EQ(len, result1.size());
    ASSERT_EQ(memcmp(buffer.get(), result1.data(), len), 0);

    //Waiting to sync logs
    mock.sync_wait();
    std::cout << "Download file...\n";

	mock.download_data(4, channel.id(), "test data", tmp_ofn);
	auto result = vds::file::read_all(tmp_ofn);

    mock.stop();
   
    ASSERT_EQ(len, result.size());
    ASSERT_EQ(memcmp(buffer.get(), result.data(), len), 0);
  }
  catch (const std::exception & ex) {
    GTEST_FAIL() << ex.what();
  }
  catch(...){
    GTEST_FAIL() << "Unexpected error";
  }
}