#include "stdafx.h"
#include "test_datacoin_protocol.h"
#include "crypto_service.h"
#include "server.h"
#include "cert_control.h"
#include "user_manager.h"
#include "db_model.h"
#include "user_wallet.h"
#include "member_user.h"
#include "transaction_log_vote_request_dbo.h"
#include "messages/transaction_log_messages.h"
#include "transaction_log.h"
#include "dht_network_client.h"

static vds::async_task<vds::expected<std::shared_ptr<vds::user_manager>>> create_user(
  const vds::service_provider * sp,
  const std::string & user_name,
  const std::string & user_email,
  const std::string & user_password) {
  CHECK_EXPECTED_ASYNC(co_await vds::user_manager::create_user(
    sp,
    user_name,
    user_email,
    user_password));

  auto user_mng = std::make_shared<vds::user_manager>(sp);
  CHECK_EXPECTED_ASYNC(co_await sp->get<vds::db_model>()->async_transaction([user_mng, user_email, user_password](vds::database_transaction & t) -> vds::expected<void> {
    return user_mng->load(t, user_email, user_password);
  }));

  co_return user_mng;
}

TEST(test_vds_dht_network, test_datacoin_protocol) {

  GET_EXPECTED_GTEST(current_process, vds::filename::current_process());
  auto folder = vds::foldername(vds::foldername(current_process.contains_folder(), "test_datacoin_protocol"));
  CHECK_EXPECTED_GTEST(folder.delete_folder(true));
  CHECK_EXPECTED_GTEST(folder.create());

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

  registrator.current_user(folder);
  registrator.local_machine(folder);

  GET_EXPECTED_GTEST(sp, registrator.build());
  CHECK_EXPECTED_GTEST(registrator.start());

  std::string root_user_name = "root";
  std::string root_password = "123123";

  vds::cert_control::private_info_t private_info;
  CHECK_EXPECTED_GTEST(private_info.genereate_all());
  CHECK_EXPECTED_GTEST(vds::cert_control::genereate_all(private_info));

  CHECK_EXPECTED_GTEST(std::make_shared<vds::user_manager>(sp)->reset(root_user_name, root_password, private_info));
  /*      0 - creare root                     root_user MAX
   */

  auto root_user_mng = std::make_shared<vds::user_manager>(sp);
  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_transaction([root_user_mng, root_user_name, root_password](vds::database_transaction & t) -> vds::expected<void> {
    return root_user_mng->load(t, root_user_name, root_password);
  }).get());

  //Create User 1
  GET_EXPECTED_GTEST(user1_mng, create_user(sp, "User1", "user1@domain.ru", "1234567").get());
  /*      0 - creare root                     root_user MAX
   *      1 - creare user1                    root_user MAX
   */

  vds::const_data_buffer root_source;
  vds::const_data_buffer first_source;
  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_transaction(
    [sp, root_user_mng, user1_mng, &root_source, &first_source](vds::database_transaction & t) -> vds::expected<void> {
    GET_EXPECTED(balance, vds::user_wallet::get_balance(t));
    vds_assert(balance.account_state().size() == 1);
    vds_assert(balance.account_state().begin()->second.balance_.size() == 1);
    root_source = balance.account_state().begin()->second.balance_.begin()->first;

    GET_EXPECTED(transaction_log, vds::transactions::transaction_block_builder::create(sp, t));
    CHECK_EXPECTED(vds::user_wallet::transfer(transaction_log, root_source, user1_mng->get_current_user(), 100));

    GET_EXPECTED_VALUE(first_source, sp->get<vds::dht::network::client>()->save(
      sp,
      transaction_log,
      t));
    return vds::expected<void>();
  }).get());
  /*      0 - (root_source, consensus) creare root              root_user MAX +
   *      1 - (consensus) creare user1                                      root_user MAX +
   *      2 - (first_source, consensus, leaf) root_user->user1: 100        root_user MAX-100 +  user1 100 -
   */


  auto root_user_account = root_user_mng->get_current_user().user_certificate()->subject();
  auto user1_account = user1_mng->get_current_user().user_certificate()->subject();

  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_read_transaction(
    [sp, root_user_mng, root_user_account, user1_mng, root_source, user1_account, first_source](vds::database_read_transaction & t) -> vds::expected<void>{
    GET_EXPECTED(balance, vds::user_wallet::get_balance(t));
    vds_assert(balance.account_state().size() == 2);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 100);

    vds::orm::transaction_log_record_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.id, t1.state, t1.consensus, t1.new_member)));
    WHILE_EXPECTED(st.execute()) {
      if(t1.id.get(st) == root_source) {
        vds_assert(t1.new_member.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
        vds_assert(t1.consensus.get(st));
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(!t1.new_member.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
        vds_assert(t1.consensus.get(st));
      }
    }
    WHILE_EXPECTED_END()

    vds::orm::transaction_log_vote_request_dbo t2;
    GET_EXPECTED_VALUE(st, t.get_reader(t2.select(t2.owner, t2.approved).where(t2.id == first_source)));
    WHILE_EXPECTED (st.execute()) {
      vds_assert(t2.owner.get(st) == root_user_account);
      vds_assert(t2.approved.get(st));
    }
    WHILE_EXPECTED_END()

    return vds::expected<void>();
  }).get());
  ///////////////////////
  GET_EXPECTED_GTEST(user2_mng, create_user(sp, "User2", "user2@domain.ru", "1234567").get());
  auto user2_account = user2_mng->get_current_user().user_certificate()->subject();

  vds::const_data_buffer log12;
  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_read_transaction([sp, root_user_mng, user1_mng, first_source, user2_mng, &log12](vds::database_read_transaction & t) -> vds::expected<void>{
    GET_EXPECTED(transaction_log, vds::transactions::transaction_block_builder::create(sp, t));
    CHECK_EXPECTED(vds::user_wallet::transfer(transaction_log, first_source, user2_mng->get_current_user(), 50));
    GET_EXPECTED_VALUE(log12, transaction_log.sign(
      sp,
      user1_mng->get_current_user().user_certificate(),
      user1_mng->get_current_user().private_key()));
    return vds::expected<void>();
  }).get());
  /*      0 - (root_source, consensus) creare root       root_user MAX +
   *      1 - creare user1                               root_user MAX +
   *      2 - (first_source) root_user->user1: 100       root_user MAX-100 +  user1 100 -
   *      3 - (leaf) create user2                        root_user MAX-100 +  user1 100 - 
   *      |\
   *      | * (log12) user1->user2:50                    root_user MAX-100 -  user1 50 +   user2 50 -
   */


  GET_EXPECTED_GTEST(user3_mng, create_user(sp, "User3", "user3@domain.ru", "1234567").get());
  auto user3_account = user3_mng->get_current_user().user_certificate()->subject();

  vds::const_data_buffer log13;
  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_read_transaction([sp, root_user_mng, user1_mng, first_source, user3_mng, &log13](vds::database_read_transaction & t) -> vds::expected<void>{
    GET_EXPECTED(transaction_log, vds::transactions::transaction_block_builder::create(sp, t));
    CHECK_EXPECTED(vds::user_wallet::transfer(transaction_log, first_source, user3_mng->get_current_user(), 50));
    GET_EXPECTED_VALUE(log13, transaction_log.sign(
      sp,
      user1_mng->get_current_user().user_certificate(),
      user1_mng->get_current_user().private_key()));
    return vds::expected<void>();
  }).get());
  /*      0 - (root_source, consensus) creare root       root_user MAX +
   *      1 - creare user1                               root_user MAX +
   *      2 - (first_source) root_user->user1: 100       root_user MAX-100 +  user1 100 -
   *      3 -        create user2                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log12) user1->user2:50                    root_user MAX-100 -  user1 50 +   user2 50 -
   *      4 - (leaf) create user3                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log13) user1->user3:50                    root_user MAX-100 -  user1 50 +   user3 50 -
   */

  GET_EXPECTED_GTEST(user4_mng, create_user(sp, "User4", "user4@domain.ru", "1234567").get());
  auto user4_account = user4_mng->get_current_user().user_certificate()->subject();

  vds::const_data_buffer log14;
  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_read_transaction([sp, root_user_mng, user1_mng, first_source, user4_mng, &log14](vds::database_read_transaction & t) -> vds::expected<void> {
    GET_EXPECTED(transaction_log, vds::transactions::transaction_block_builder::create(sp, t));
    CHECK_EXPECTED(vds::user_wallet::transfer(transaction_log, first_source, user4_mng->get_current_user(), 50));
    GET_EXPECTED(log14, transaction_log.sign(
      sp,
      user1_mng->get_current_user().user_certificate(),
      user1_mng->get_current_user().private_key()));
    return vds::expected<void>();
  }).get());
  /*      0 - (root_source, consensus) creare root       root_user MAX +
   *      1 - creare user1                               root_user MAX +
   *      2 - (first_source) root_user->user1: 100       root_user MAX-100 +  user1 100 -
   *      3 - (leaf) create user2                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log12) user1->user2:50                    root_user MAX-100 -  user1 50 +   user2 50 -
   *      4 -        create user3                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log13) user1->user3:50                    root_user MAX-100 -  user1 50 +   user3 50 -
   *      5 - (leaf) create user4                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log14) user1->user4:50                    root_user MAX-100 -  user1 50 +   user4 50 -
   */

  //Validate
  vds::const_data_buffer log12_id;
  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_transaction(
    [sp, root_user_mng, root_user_account, user1_mng, root_source, user1_account, user2_account, first_source, log12, &log12_id](vds::database_transaction & t) -> vds::expected<void>{
    GET_EXPECTED_VALUE(log12_id, vds::transactions::transaction_log::save(sp, t, log12));

    GET_EXPECTED(balance, vds::user_wallet::get_balance(t));
    vds_assert(balance.account_state().size() == 3);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 50);
    vds_assert(balance.account_state().at(user2_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user2_account).balance_.at(log12_id) == 50);

    vds::orm::transaction_log_record_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.id, t1.state, t1.consensus)));
    WHILE_EXPECTED (st.execute()) {
      if (t1.id.get(st) == root_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == log12_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
    }
    WHILE_EXPECTED_END()

    return vds::expected<void>();
  }).get());
  /*      0 - (root_source, consensus) creare root       root_user MAX +
   *      1 - creare user1                               root_user MAX +
   *      2 - (first_source) root_user->user1: 100       root_user MAX-100 +  user1 100 -
   *      3 - (leaf) create user2                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log12,leaf) user1->user2:50               root_user MAX-100 -  user1 50 +   user2 50 -
   *      4 -        create user3                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log13) user1->user3:50                    root_user MAX-100 -  user1 50 +   user3 50 -
   *      5 - (leaf) create user4                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log14) user1->user4:50                    root_user MAX-100 -  user1 50 +   user4 50 -
   */

  vds::const_data_buffer log13_id;
  CHECK_EXPECTED_GTEST(sp->get<vds::db_model>()->async_transaction(
    [
      sp,
      root_user_mng,
      root_user_account,
      user1_mng,
      root_source,
      user1_account,
      user2_account,
      user3_account,
      first_source,
      log13,
      log12_id,
      &log13_id](vds::database_transaction & t) -> vds::expected<void> {
    GET_EXPECTED_VALUE(log13_id, vds::transactions::transaction_log::save(sp, t, log13));

    GET_EXPECTED(balance, vds::user_wallet::get_balance(t));
    vds_assert(balance.account_state().size() == 4);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 0);
    vds_assert(balance.account_state().at(user2_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user2_account).balance_.at(log12_id) == 50);
    vds_assert(balance.account_state().at(user3_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user3_account).balance_.at(log13_id) == 50);

    vds::orm::transaction_log_record_dbo t1;
    GET_EXPECTED(st, t.get_reader(t1.select(t1.id, t1.state, t1.consensus)));
    WHILE_EXPECTED (st.execute()) {
      if (t1.id.get(st) == root_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == log12_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
      else if (t1.id.get(st) == log13_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
    }
    WHILE_EXPECTED_END()

    return vds::expected<void>();
  }).get());
  /*      0 - (root_source, consensus) creare root       root_user MAX +
   *      1 - creare user1                               root_user MAX +
   *      2 - (first_source) root_user->user1: 100       root_user MAX-100 +  user1 100 -
   *      3 - (leaf) create user2                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log12,leaf) user1->user2:50               root_user MAX-100 -  user1 50 +   user2 50 -
   *      4 -        create user3                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log13,leaf) user1->user3:50               root_user MAX-100 -  user1 50 +   user3 50 -
   *      5 - (leaf) create user4                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log14) user1->user4:50                    root_user MAX-100 -  user1 50 +   user4 50 -
   */
  /*
  //test invalid
  vds::const_data_buffer log14_id;
  sp->get<vds::db_model>()->async_transaction(
    [
      sp,
      root_user_mng,
      root_user_account,
      user1_mng,
      root_source,
      user1_account,
      user2_account,
      user3_account,
      first_source,
      log14,
      log12_id,
      log13_id,
      &log14_id](vds::database_transaction & t) {
    log14_id = vds::transactions::transaction_log::save(sp, t, log14);

    auto balance = vds::user_wallet::safe_get_balance(sp, t);
    vds_assert(balance.account_state().size() == 4);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 0);

    vds::orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(t1.id, t1.state, t1.consensus));
    while (st.execute()) {
      if (t1.id.get(st) == root_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == log12_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf
        || t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::invalid);
      }
      else if (t1.id.get(st) == log13_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf
          || t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::invalid);
      }
      else if (t1.id.get(st) == log14_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf
          || t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::invalid);
      }
    }
  }).get();
  */
  /*      0 - (root_source, consensus) creare root       root_user MAX +
   *      1 - creare user1                               root_user MAX +
   *      2 - (first_source) root_user->user1: 100       root_user MAX-100 +  user1 100 -
   *      3 - (leaf) create user2                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log12,leaf) user1->user2:50               root_user MAX-100 -  user1 50 +   user2 50 -
   *      4 -        create user3                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log13,leaf) user1->user3:50               root_user MAX-100 -  user1 50 +               user3 50 -
   *      5 - (leaf) create user4                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | * (log14,leaf) user1->user4:50               root_user MAX-100 -  user1 50 +                           user4 50 -
   */
  /*
  vds::const_data_buffer log14_log12_id;
  sp->get<vds::db_model>()->async_transaction(
    [
      sp,
      root_user_mng,
      root_user_account,
      user1_mng,
      user4_mng,
      root_source,
      user1_account,
      user2_account,
      user3_account,
      first_source,
      log14,
      log12_id,
      log13_id,
      log14_id,
      &log14_log12_id](vds::database_transaction & t) {
    std::set<vds::const_data_buffer> ancestors;
    ancestors.emplace(log12_id);
    ancestors.emplace(log14_id);
    vds::transactions::transaction_block_builder transaction_log(sp, t, ancestors);
    vds::user_wallet::transfer(transaction_log, first_source, user4_mng->get_current_user(), 50);
    log14_log12_id = transaction_log.sign(
      sp,
      user1_mng->get_current_user().user_certificate(),
      user1_mng->get_current_user().private_key());

    auto balance = vds::user_wallet::get_balance(t);
    vds_assert(balance.account_state().size() == 4);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 0);

    vds::orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(t1.id, t1.state, t1.consensus));
    while (st.execute()) {
      if (t1.id.get(st) == root_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::processed);
      }
      else if (t1.id.get(st) == log12_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
      else if (t1.id.get(st) == log13_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
      else if (t1.id.get(st) == log14_id) {
        vds_assert(!t1.consensus.get(st));
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::invalid);
      }
    }
  }).get();
  */
  /*      0 - (root_source, consensus) creare root       root_user MAX +
   *      1 - creare user1                               root_user MAX +
   *      2 - (first_source) root_user->user1: 100       root_user MAX-100 +  user1 100 -
   *      3 - (leaf) create user2                        root_user MAX-100 +  user1 100 -
   *      |\
   *      | | (log12) user1->user2:50                    root_user MAX-100 -  user1 50 +   user2 50 -
   *      4-+-       create user3                        root_user MAX-100 +  user1 100 -
   *      | | \
   *      | |   * (log13,leaf) user1->user3:50           root_user MAX-100 -  user1 50 +               user3 50 -
   *      5-+-    (leaf) create user4                    root_user MAX-100 +  user1 100 -
   *      | | \
   *      | |  * (log14,invalid) user1->user4:50         root_user MAX-100 -  user1 50 +                           user4 50 -
   *      | | /
   *      | *    (log14_log12, leaf)
   */
  CHECK_EXPECTED_GTEST(registrator.shutdown());
}