//
// Created by vadim on 13.06.18.
//
#include "stdafx.h"
#include "vds_embedded.h"

#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring

JNICALL
Java_ru_ivysoft_vds_CameraActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

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

void vds::vds_embedded::set_root_folder(const std::string & root_folder) {
  this->root_folder_ = root_folder;
}
