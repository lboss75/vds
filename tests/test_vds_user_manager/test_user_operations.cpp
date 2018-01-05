#include "stdafx.h"
#include "user_manager.h"
#include "user_manager_storage.h"
#include "member_user.h"
#include "asymmetriccrypto.h"

class test_user_manager_storage : public vds::iuser_manager_storage
{
public:
  vds::member_user new_user(vds::member_user && user) override
  {
    return std::move(user);
  }

  vds::user_channel new_channel(
      vds::user_channel &&channel,
      const vds::guid &owner_id_,
      const vds::const_data_buffer &crypted_private_key) override
  {
    return std::move(channel);
  }

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
  auto user1 = root_user.create_device_user(root_private_key, "test1", "123qwe", <#initializer#>);

  vds::asymmetric_private_key user2_private_key(vds::asymmetric_crypto::rsa2048());
  user2_private_key.generate();
  auto user2 = root_user.create_device_user(root_private_key, "test2", "123qwe", <#initializer#>);
  
  auto channel1 = manager.create_channel(<#initializer#>, <#initializer#>, user1, user1_private_key, "channel1",
                                         <#initializer#>,
                                         <#initializer#>);
  auto channel2 = manager.create_channel(<#initializer#>, <#initializer#>, user2, user2_private_key, "channel2",
                                         <#initializer#>,
                                         <#initializer#>);
  
  auto channel3 = manager.create_channel(<#initializer#>, <#initializer#>, user1, user1_private_key, "channel3",
                                         <#initializer#>,
                                         <#initializer#>);
  //channel3.add_user(user2);
  
  char test_data1[] = "test message1";
  auto message1 = channel1.crypt_message(user1, vds::const_data_buffer((const uint8_t *)test_data1, sizeof(test_data1) - 1));
  
  
  
}

