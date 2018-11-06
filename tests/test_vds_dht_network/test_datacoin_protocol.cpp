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

static vds::async_task<std::shared_ptr<vds::user_manager>> create_user(
  const vds::service_provider * sp,
  std::shared_ptr<vds::user_manager> root_user_mng,
  const std::string & user_name,
  const std::string & user_email,
  const std::string & user_password) {
  auto user_request = vds::user_manager::create_register_request(
    sp,
    user_name,
    user_email,
    user_password);

  auto is_ok = co_await root_user_mng->approve_join_request(user_request);
  vds_assert(is_ok);

  auto user_mng = std::make_shared<vds::user_manager>(sp);
  co_await sp->get<vds::db_model>()->async_transaction([user_mng, user_email, user_password](vds::database_transaction & t) {
    user_mng->load(t, user_email, user_password);
  });

  co_return user_mng;
}

TEST(test_vds_dht_network, test_datacoin_protocol) {

  auto folder = vds::foldername(vds::foldername(vds::filename::current_process().contains_folder(), "test_datacoin_protocol"));
  folder.delete_folder(true);
  folder.create();

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

  //Create User 1
  auto user1_mng = create_user(sp, root_user_mng, "User1", "user1@domain.ru", "1234567").get();

  vds::const_data_buffer root_source;
  vds::const_data_buffer first_source;
  sp->get<vds::db_model>()->async_transaction([sp, root_user_mng, user1_mng, &root_source, &first_source](vds::database_transaction & t) {
    auto balance = vds::user_wallet::get_balance(t);
    vds_assert(balance.account_state().size() == 1);
    vds_assert(balance.account_state().begin()->second.balance_.size() == 1);
    root_source = balance.account_state().begin()->second.balance_.begin()->first;

    vds::transactions::transaction_block_builder transaction_log(sp, t);
    vds::user_wallet::transfer(transaction_log, root_source, user1_mng->get_current_user(), 100);
    first_source = transaction_log.save(
      sp,
      t,
      root_user_mng->get_current_user().user_certificate(),
      root_user_mng->get_current_user().private_key());
  }).get();

  auto root_user_account = root_user_mng->get_current_user().user_certificate()->subject();
  auto user1_account = user1_mng->get_current_user().user_certificate()->subject();

  sp->get<vds::db_model>()->async_read_transaction(
    [sp, root_user_mng, root_user_account, user1_mng, root_source, user1_account, first_source](vds::database_read_transaction & t) {
    auto balance = vds::user_wallet::get_balance(t);
    vds_assert(balance.account_state().size() == 2);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 100);

    vds::orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(t1.id, t1.state));
    while(st.execute()) {
      if(t1.id.get(st) == root_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::consensus);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
    }

    vds::orm::transaction_log_vote_request_dbo t2;
    st = t.get_reader(t2.select(t2.owner, t2.is_appoved).where(t2.id == first_source));
    while(st.execute()) {
      if(t2.owner.get(st) == root_user_account) {
        vds_assert(t2.is_appoved.get(st));
      }
      else if (t2.owner.get(st) == root_user_account) {
        vds_assert(!t2.is_appoved.get(st));
      }
    }
  }).get();
  ///////////////////////
  auto user2_mng = create_user(sp, root_user_mng, "User2", "user2@domain.ru", "1234567").get();
  auto user2_account = user2_mng->get_current_user().user_certificate()->subject();

  vds::const_data_buffer log12;
  sp->get<vds::db_model>()->async_read_transaction([sp, root_user_mng, user1_mng, first_source, user2_mng, &log12](vds::database_read_transaction & t) {
    vds::transactions::transaction_block_builder transaction_log(sp, t);
    vds::user_wallet::transfer(transaction_log, first_source, user2_mng->get_current_user(), 50);
    log12 = transaction_log.sign(
      sp,
      user1_mng->get_current_user().user_certificate(),
      user1_mng->get_current_user().private_key());
  }).get();

  auto user3_mng = create_user(sp, root_user_mng, "User3", "user3@domain.ru", "1234567").get();
  auto user3_account = user3_mng->get_current_user().user_certificate()->subject();

  vds::const_data_buffer log13;
  sp->get<vds::db_model>()->async_read_transaction([sp, root_user_mng, user1_mng, first_source, user3_mng, &log13](vds::database_read_transaction & t) {
    vds::transactions::transaction_block_builder transaction_log(sp, t);
    vds::user_wallet::transfer(transaction_log, first_source, user3_mng->get_current_user(), 50);
    log13 = transaction_log.sign(
      sp,
      user1_mng->get_current_user().user_certificate(),
      user1_mng->get_current_user().private_key());
  }).get();

  auto user4_mng = create_user(sp, root_user_mng, "User4", "user4@domain.ru", "1234567").get();
  auto user4_account = user4_mng->get_current_user().user_certificate()->subject();

  vds::const_data_buffer log14;
  sp->get<vds::db_model>()->async_read_transaction([sp, root_user_mng, user1_mng, first_source, user4_mng, &log14](vds::database_read_transaction & t) {
    vds::transactions::transaction_block_builder transaction_log(sp, t);
    vds::user_wallet::transfer(transaction_log, first_source, user4_mng->get_current_user(), 50);
    log14 = transaction_log.sign(
      sp,
      user1_mng->get_current_user().user_certificate(),
      user1_mng->get_current_user().private_key());
  }).get();

  //Validate
  vds::const_data_buffer log12_id;
  sp->get<vds::db_model>()->async_transaction(
    [sp, root_user_mng, root_user_account, user1_mng, root_source, user1_account, user2_account, first_source, log12, &log12_id](vds::database_transaction & t) {
    log12_id = vds::transactions::transaction_log::save(sp, t, log12);

    auto balance = vds::user_wallet::get_balance(t);
    vds_assert(balance.account_state().size() == 3);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 50);
    vds_assert(balance.account_state().at(user2_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user2_account).balance_.at(log12_id) == 50);

    vds::orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(t1.id, t1.state));
    while (st.execute()) {
      if (t1.id.get(st) == root_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::consensus);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::consensus);
      }
      else if (t1.id.get(st) == log12_id) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
    }
  }).get();

  vds::const_data_buffer log13_id;
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
      log13,
      log12_id,
      &log13_id](vds::database_transaction & t) {
    log13_id = vds::transactions::transaction_log::save(sp, t, log13);

    auto balance = vds::user_wallet::get_balance(t);
    vds_assert(balance.account_state().size() == 4);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 0);
    vds_assert(balance.account_state().at(user2_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user2_account).balance_.at(log12_id) == 50);
    vds_assert(balance.account_state().at(user3_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user3_account).balance_.at(log13_id) == 50);

    vds::orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(t1.id, t1.state));
    while (st.execute()) {
      if (t1.id.get(st) == root_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::consensus);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::consensus);
      }
      else if (t1.id.get(st) == log12_id) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
      else if (t1.id.get(st) == log13_id) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
    }
  }).get();

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

    auto balance = vds::user_wallet::get_balance(t);
    vds_assert(balance.account_state().size() == 4);
    vds_assert(balance.account_state().at(root_user_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user1_account).balance_.at(first_source) == 0);
    vds_assert(balance.account_state().at(user2_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user2_account).balance_.at(log12_id) == 50);
    vds_assert(balance.account_state().at(user3_account).balance_.size() == 1);
    vds_assert(balance.account_state().at(user3_account).balance_.at(log13_id) == 50);

    vds::orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(t1.id, t1.state));
    while (st.execute()) {
      if (t1.id.get(st) == root_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::consensus);
      }
      else if (t1.id.get(st) == first_source) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::consensus);
      }
      else if (t1.id.get(st) == log12_id) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
      else if (t1.id.get(st) == log13_id) {
        vds_assert(t1.state.get(st) == vds::orm::transaction_log_record_dbo::state_t::leaf);
      }
    }
  }).get();

  registrator.shutdown();

}