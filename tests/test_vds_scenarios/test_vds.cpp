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

    mock.upload_file(3, "test data", buffer.get(), len);
    
    //Waiting to sync logs
    mock.sync_wait();
    std::cout << "Download file...\n";

    auto result = mock.download_data(4, "test data");

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