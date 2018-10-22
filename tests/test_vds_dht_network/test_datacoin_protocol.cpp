#include "stdafx.h"
#include "test_datacoin_protocol.h"
#include "crypto_service.h"
#include "server.h"
#include "cert_control.h"
#include "user_manager.h"
#include "db_model.h"
#include "user_wallet.h"

TEST(test_vds_dht_network, test_datacoin_protocol) {

  vds::service_registrator registrator;

  vds::console_logger logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::mt_service mt_service;
  
  vds::task_manager task_manager;
  task_manager.disable_timers();

  vds::crypto_service crypto_service;
  vds::server server;

  registrator.add(logger);
  registrator.add(mt_service);
  registrator.add(task_manager);
  registrator.add(crypto_service);
  registrator.add(server);

  auto sp = registrator.build();
  registrator.start();

  std::string root_user_name = "root";
  std::string root_password = "123123";

  vds::cert_control::private_info_t private_info;
  private_info.genereate_all();
  vds::cert_control::genereate_all(root_user_name, root_password, private_info);

  std::make_shared<vds::user_manager>(sp)->reset(root_user_name, root_password, private_info);

  auto root_user_mng = std::make_shared<vds::user_manager>(sp);
  sp->get<vds::db_model>()->async_transaction([root_user_mng, root_user_name, root_password](vds::database_transaction & t) {
    root_user_mng->load(t, root_user_name, root_password);
  }).get();

  //User 1
  auto user1_request = vds::user_manager::create_register_request(
    sp,
    "User1",
    "user1@domain.com",
    "1234567").get();

  auto is_ok = root_user_mng->approve_join_request(user1_request).get();
  GTEST_ASSERT_EQ(is_ok, true);

  auto user1_mng = std::make_shared<vds::user_manager>(sp);
  sp->get<vds::db_model>()->async_transaction([user1_mng](vds::database_transaction & t) {
    user1_mng->load(t, "user1@domain.com", "1234567");
  }).get();

  //
  //vds::user_wallet::transfer(user1_mng->get_current_user(), root_user_mng, 100);

  //auto user1 = user_manager::create_user();
  //auto user2 = user_manager::create_user();
  //auto user3 = user_manager::create_user();

  //user_wallet w1 = user_wallet::create_wallet(user1);
  //user_wallet w2 = user_wallet::create_wallet(user2);
  //user_wallet w3 = user_wallet::create_wallet(user3);

  //w1.move_coin(root_user, 200);

  registrator.shutdown();

}