//
// Created by vadim on 13.06.18.
//
#include "stdafx.h"
#include "vds_embedded.h"

void vds::vds_embedded::server_root(const std::string & login, const std::string & password) {

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
    vds::foldername(folder, ".vds").create();

    registrator.add(mt_service);
    registrator.add(logger);
    registrator.add(task_manager);
    registrator.add(crypto_service);
    registrator.add(network_service);
    registrator.add(server);

    std::shared_ptr<std::exception> error;

    auto sp = registrator.build("server::init_root");
    sp.set_property<vds::unhandled_exception_handler>(
        vds::service_provider::property_scope::any_scope,
        new vds::unhandled_exception_handler(
            [&error](const vds::service_provider &sp, const std::shared_ptr<std::exception> &ex) {
              error = ex;
            }));
    try {
      auto root_folders = new vds::persistence_values();
      root_folders->current_user_ = folder;
      root_folders->local_machine_ = folder;
      sp.set_property<vds::persistence_values>(vds::service_provider::property_scope::root_scope, root_folders);

      registrator.start(sp);

      vds::imt_service::enable_async(sp);
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
      try { registrator.shutdown(sp); }
      catch (...) {}

      throw;
    }

    registrator.shutdown(sp);

    if (error) {
      throw std::runtime_error(error->what());
    }
  }
  catch(const std::exception & ex){
    this->last_error_ = ex.what();
  }
}

void vds::vds_embedded::set_root_folder(const std::string & root_folder) {
  this->root_folder_ = root_folder;
}
