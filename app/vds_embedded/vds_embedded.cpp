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
void vds::vds_embedded::server_root(const std::string & /*login*/, const std::string & /*password*/) {

  this->last_error_.clear();
  try {
    if (this->root_folder_.empty()) {
      throw std::runtime_error("Set root folder");
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
    folder.delete_folder(true);
    folder.create();

    registrator.add(mt_service);
    registrator.add(logger);
    registrator.add(task_manager);
    registrator.add(crypto_service);
    registrator.add(network_service);
    registrator.add(server);

    registrator.current_user(folder);
    registrator.local_machine(folder);

    auto sp = registrator.build();
    try {
      registrator.start();

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

    }
    catch (...) {
      try { registrator.shutdown(); }
      catch (...) {}

      throw;
    }

    registrator.shutdown();
  }
  catch(const std::exception & ex){
    this->last_error_ = ex.what();
  }
}

void vds::vds_embedded::start(int port) {
  if (!this->root_folder_.empty()) {
    auto folder = foldername(this->root_folder_);
    folder.delete_folder(true);
    folder.create();
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

  this->sp_ = this->registrator_.build();
  this->registrator_.start();

  this->server_.start_network(port).get();
}

void vds::vds_embedded::stop() {
  this->registrator_.shutdown();
}

bool vds::vds_embedded::local_storage_exists() {
  return user_storage::local_storage_exists(this->sp_).get();
}

vds::vds_embedded::vds_session::vds_session(
  service_provider * sp,
  const std::shared_ptr<user_manager>& user_mng)
: sp_(sp), user_mng_(user_mng) {
}

const char* vds::vds_embedded::vds_session::get_login_state() const {

  this->user_mng_->update().get();

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

  this->last_result_ = user_storage::device_storages(
    this->sp_,
    this->user_mng_).get()->json_value::str();
  return this->last_result_.c_str();

}

const char* vds::vds_embedded::vds_session::prepare_device_storage() {
  this->last_result_ = user_storage::device_storage_label(this->user_mng_)->json_value::str();
  return this->last_result_.c_str();
}

const char* vds::vds_embedded::vds_session::add_device_storage(
  const std::string& name,
  const std::string& local_path,
  uint64_t reserved_size) {

  user_storage::add_device_storage(this->sp_, this->user_mng_, name, local_path, reserved_size).get();
  return nullptr;
}

vds::vds_embedded::vds_session * vds::vds_embedded::login(
  const std::string& login,
  const std::string& password) {

  auto user_mng = std::make_shared<user_manager>(this->sp_);
  this->sp_->get<db_model>()->async_read_transaction([user_mng, login, password](database_read_transaction & t) {
    user_mng->load(t, login, password);
  }).get();

  return new vds_session(this->sp_, user_mng);

}

vds::vds_embedded::vds_embedded()
: logger_(log_level::ll_trace, { "*" }) {
}

void vds::vds_embedded::set_root_folder(const std::string & root_folder) {
  this->root_folder_ = root_folder;
}
