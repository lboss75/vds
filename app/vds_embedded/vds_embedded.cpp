//
// Created by vadim on 13.06.18.
//
#include "stdafx.h"
#include "vds_embedded.h"
#include "user_manager.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "device_config_dbo.h"
#include "device_record_dbo.h"
#include "member_user.h"
#include "user_storage.h"

/*

#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring

JNICALL
Java_ru_ivysoft_vds_CameraActivity_stringFromJNI(
        JNIEnv *env,
        jobject / * this * / ) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
*/
void vds::vds_embedded::server_root(const std::string & login, const std::string & password) {
  this->last_error_.clear();
  auto result = do_server_root(login, password);
  if(result.has_error()) {
    this->last_error_ = result.error()->what();
  }
}

vds::expected<void> vds::vds_embedded::do_server_root(const std::string & /*login*/, const std::string & /*password*/) {

  if (this->root_folder_.empty()) {
    return vds::make_unexpected<std::runtime_error>("Set root folder");
  }

  service_registrator registrator;

  mt_service mt_service;
  network_service network_service;

  std::unordered_set<std::string> modules;
  modules.emplace("*");
  file_logger logger(log_level::ll_trace, modules);

  crypto_service crypto_service;
  task_manager task_manager;
  server server;

  auto folder = foldername(this->root_folder_);
  CHECK_EXPECTED(folder.delete_folder(true));
  CHECK_EXPECTED(folder.create());

  registrator.add(mt_service);
  registrator.add(logger);
  registrator.add(task_manager);
  registrator.add(crypto_service);
  registrator.add(network_service);
  registrator.add(server);

  registrator.current_user(folder);
  registrator.local_machine(folder);

  //auto sp = registrator.build();
  CHECK_EXPECTED(registrator.start());

  vds::barrier b;
  //server
  //    .reset(sp, login, password)
  //    .execute([&error, &b](const std::shared_ptr<std::exception> &ex) {
  //      if (ex) {
  //        error = ex;
  //      }
  //      b.set();
  //    });

  b.wait();

  CHECK_EXPECTED(registrator.shutdown());

  return expected<void>();
}

void vds::vds_embedded::start(int port, bool dev_network) {
  this->last_error_.clear();
  auto result = do_start(port, dev_network);
  if (result.has_error()) {
    this->last_error_ = result.error()->what();
  }
}

vds::expected<void> vds::vds_embedded::do_start(int port, bool dev_network) {
  if (!this->root_folder_.empty()) {
    auto folder = foldername(this->root_folder_);
    CHECK_EXPECTED(folder.delete_folder(true));
    CHECK_EXPECTED(folder.create());
  }

  this->registrator_.add(this->mt_service_);
  this->registrator_.add(this->logger_);
  this->registrator_.add(this->task_manager_);
  this->registrator_.add(this->crypto_service_);
  this->registrator_.add(this->network_service_);
  this->registrator_.add(this->server_);

  if (!this->root_folder_.empty()) {
    auto folder = foldername(this->root_folder_);
    this->registrator_.current_user(folder);
    this->registrator_.local_machine(folder);
  }

  GET_EXPECTED_VALUE(this->sp_, this->registrator_.build());
  CHECK_EXPECTED(this->registrator_.start());

  CHECK_EXPECTED(this->server_.start_network(port, dev_network).get());

  return expected<void>();
}

void vds::vds_embedded::stop() {
  this->last_error_.clear();
  auto result = do_stop();
  if (result.has_error()) {
    this->last_error_ = result.error()->what();
  }
}

vds::expected<void> vds::vds_embedded::do_stop() {
  return this->registrator_.shutdown();
}

bool vds::vds_embedded::local_storage_exists() {
  this->last_error_.clear();
  auto result = do_local_storage_exists();
  if (result.has_error()) {
    this->last_error_ = result.error()->what();
    return false;
  }
  else {
    return result.value();
  }
}

vds::expected<bool> vds::vds_embedded::do_local_storage_exists() {
  return user_storage::local_storage_exists(this->sp_).get();
}

vds::vds_embedded::vds_session::vds_session(
  service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng)
: sp_(sp), user_mng_(user_mng) {
}

const char* vds::vds_embedded::vds_session::get_login_state() {
  auto result = this->user_mng_->update().get();
  if(result.has_error()) {
    this->last_result_ = result.error()->what();
    return this->last_result_.c_str();
  }

  switch(this->user_mng_->get_login_state()) {
  case user_manager::login_state_t::login_failed:
    return "failed";
  case user_manager::login_state_t::login_successful:
    return "successful";
  default:
    return "waiting";
  }
}

const char* vds::vds_embedded::vds_session::get_device_storages() {
  auto result = user_storage::device_storages(
    this->sp_,
    this->user_mng_).get();
  if(result.has_error()) {
    this->last_result_ = result.error()->what();
  }
  else {
    auto result_str = result.value()->json_value::str();
    if (result_str.has_error()) {
      this->last_result_ = result_str.error()->what();
    }
    else {
      this->last_result_ = std::move(result_str.value());
    }
  }
  return this->last_result_.c_str();

}

const char* vds::vds_embedded::vds_session::prepare_device_storage() {
  auto result = user_storage::device_storage_label(
    this->user_mng_);
  if (result.has_error()) {
    this->last_result_ = result.error()->what();
  }
  else {
    auto result_str = result.value()->json_value::str();
    if (result_str.has_error()) {
      this->last_result_ = result_str.error()->what();
    }
    else {
      this->last_result_ = std::move(result_str.value());
    }
  }
  return this->last_result_.c_str();
}

const char* vds::vds_embedded::vds_session::add_device_storage(
  const std::string& name,
  const std::string& local_path,
  uint64_t reserved_size) {

  auto result = user_storage::add_device_storage(this->sp_, this->user_mng_, name, local_path, reserved_size).get();
  if (result.has_error()) {
    this->last_result_ = result.error()->what();
    return this->last_result_.c_str();
  }
  else {
    return nullptr;
  }
}

vds::vds_embedded::vds_session * vds::vds_embedded::login(
  const std::string& login,
  const std::string& password) {

  auto user_mng = std::make_shared<user_manager>(this->sp_);
  auto result = this->sp_->get<db_model>()->async_read_transaction([user_mng, login, password](database_read_transaction & t) -> expected<void> {
    return user_mng->load(t, login, password);
  }).get();

  if (result.has_error()) {
    this->last_error_ = result.error()->what();
    return nullptr;
  }

  return new vds_session(this->sp_, user_mng);

}

vds::vds_embedded::vds_embedded()
: logger_(log_level::ll_trace, { "*" }) {
}

void vds::vds_embedded::set_root_folder(const std::string & root_folder) {
  this->root_folder_ = root_folder;
}
