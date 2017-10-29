#include "stdafx.h"

TEST(test_user_operations, test_add_user)
{
  vds::user_manager manager;
  
  auto root_user = manager.create_root_user("root", "123qwe!@#");
  
  auto user1 = manager.create_user(root_user, "test1", "123qwe");
  auto user2 = manager.create_user(root_user, "test2", "123qwe");
  
  auto channel1 = manager.create_channel(user1, "channel1");
  auto channel2 = manager.create_channel(user2, "channel2");
  
  auto channel3 = manager.create_channel(user1, "channel3");
  channel3.add_user(user2);
  
  auto message1 = channel1.crypt_message(user1, "test message1");
  
  
  
}

