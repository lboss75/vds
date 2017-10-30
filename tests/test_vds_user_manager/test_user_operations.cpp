#include "stdafx.h"
#include "user_manager.h"

class test_user_manager_storage : public vds::iuser_manager_storage
{
public:
};

TEST(test_user_operations, test_add_user)
{
  auto storage = std::make_shared<test_user_manager_storage>();

  vds::user_manager manager(storage);
  
  vds::asymmetric_private_key root_private_key(vds::asymmetric_crypto::rsa2048());
  root_private_key.generate();
  auto root_user = manager.create_root_user("root", "123qwe!@#", root_private_key);
  

  vds::asymmetric_private_key user1_private_key(vds::asymmetric_crypto::rsa2048());
  user1_private_key.generate();
  auto user1 = root_user.create_user(root_private_key, "test1", "123qwe", user1_private_key);

  vds::asymmetric_private_key user2_private_key(vds::asymmetric_crypto::rsa2048());
  user2_private_key.generate();
  auto user2 = root_user.create_user(root_private_key, "test2", "123qwe", user2_private_key);
  
  auto channel1 = manager.create_channel(user1, "channel1");
  auto channel2 = manager.create_channel(user2, "channel2");
  
  auto channel3 = manager.create_channel(user1, "channel3");
  channel3.add_user(user2);
  
  char test_data1[] = "test message1";
  auto message1 = channel1.crypt_message(user1, vds::const_data_buffer((const uint8_t *)test_data1, sizeof(test_data1) - 1));
  
  
  
}

