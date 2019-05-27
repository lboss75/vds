/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "autoupdate_service.h"
#include "user_manager.h"
#include "db_model.h"
#include "file_operations.h"

vds::autoupdate::autoupdate_service::autoupdate_service()
: update_timer_("Auto Update Service"){
}

vds::expected<void> vds::autoupdate::autoupdate_service::register_services(service_registrator&) {
  return expected<void>();
}

vds::expected<void> vds::autoupdate::autoupdate_service::start(const service_provider* sp) {
  return this->update_timer_.start(sp, std::chrono::minutes(30), [this, sp]() -> async_task<expected<bool>> {
    CHECK_EXPECTED_ASYNC(co_await this->check_update(sp));
    co_return  expected<bool>(true);
  });
}

vds::expected<void> vds::autoupdate::autoupdate_service::stop() {
  CHECK_EXPECTED(this->update_timer_.stop());
  this->user_manager_.reset();
  return expected<void>();
}

vds::async_task<vds::expected<void>> vds::autoupdate::autoupdate_service::prepare_to_stop() {
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::autoupdate::autoupdate_service::check_update(const service_provider* sp) {
  GET_EXPECTED_ASYNC(current_user_folder, persistence::current_user(sp));
  foldername update_folder(current_user_folder, "update");

  std::map<filename, const_data_buffer> file_hash;
  (void)update_folder.files([&file_hash](const filename & fn)->expected<bool> {
    
    file f;
    CHECK_EXPECTED(f.open(fn, file::file_mode::open_read));

    GET_EXPECTED(h, hash::create(hash::sha256()));
    
    uint8_t buffer[1024];
    for(;;) {
      GET_EXPECTED(readed, f.read(buffer, sizeof(buffer)));
      if(0 == readed) {
        break;
      }

      CHECK_EXPECTED(h.update(buffer, readed));
    }

    CHECK_EXPECTED(h.final());

    file_hash[fn] = h.signature();

    return true;
  });
  bool update_found = false;
  std::list<vds::transactions::user_message_transaction::file_info_t> download_tasks;
  CHECK_EXPECTED_ASYNC(co_await sp->get<db_model>()->async_read_transaction(
    [sp, this, update_folder, &file_hash, &download_tasks, &update_found](database_read_transaction & t) -> expected<void> {
    if (!this->user_manager_) {
      this->user_manager_ = std::make_shared<user_manager>(sp);
      CHECK_EXPECTED(this->user_manager_->load(
        t, cert_control::auto_update_login(), cert_control::auto_update_password()));

    }
    else {
      CHECK_EXPECTED(this->user_manager_->update(t));
    }

    if (this->user_manager_->get_login_state() == user_manager::login_state_t::login_successful) {
      CHECK_EXPECTED(this->user_manager_->walk_messages(
        cert_control::get_autoupdate_channel_id(),
        t,
        [this, update_folder, &file_hash, &download_tasks, &update_found](
          const transactions::user_message_transaction &message,
          const transactions::message_environment_t & /*message_environment*/) -> expected<bool> {
        if (!message.message) {
          return true;
        }

        auto obj = std::dynamic_pointer_cast<json_object>(message.message);
        if (!obj) {
          return true;
        }

        std::string arch;
        GET_EXPECTED(has_property, obj->get_property("arch", arch));
        if(!has_property || arch != service_provider::system_name()) {
          return true;
        }

        std::string version;
        GET_EXPECTED_VALUE(has_property, obj->get_property("min_version", version));
        if (has_property && vds::version::parse(version) > service_provider::system_version()) {
          return true;
        }

        GET_EXPECTED_VALUE(has_property, obj->get_property("max_version", version));
        if (has_property && vds::version::parse(version) <= service_provider::system_version()) {
          return true;
        }

        std::string brunch;
        GET_EXPECTED_VALUE(has_property, obj->get_property("brunch", brunch));
        if (!has_property || arch != app::brunch()) {
          return true;
        }
        GET_EXPECTED_VALUE(has_property, obj->get_property("product_version", version));
        if (!has_property || vds::version::parse(version) <= app::product_version()) {
          return true;
        }

        GET_EXPECTED_VALUE(has_property, obj->get_property("min_product_version", version));
        if (has_property && vds::version::parse(version) > app::product_version()) {
          return true;
        }

        GET_EXPECTED_VALUE(has_property, obj->get_property("max_product_version", version));
        if (has_property && vds::version::parse(version) < app::product_version()) {
          return true;
        }

        for (const auto & file : message.files) {
          auto p = file_hash.find(filename(update_folder, file.name));
          if(file_hash.end() == p || p->second != file.file_id) {
            download_tasks.push_back(file);
          }
        }
        
        update_found = true;
        return false;
      }));

      return expected<void>();
    }

    return expected<void>();
  }));

  if(update_found) {
    if (!download_tasks.empty()) {
      auto file_mananger = sp->get<file_manager::file_operations>();
      for(auto & p : download_tasks) {
        file f;
        CHECK_EXPECTED_ASYNC(f.open(filename(update_folder, p.name), file::file_mode::open_or_create));
        auto stream = std::make_shared<file_stream_output_async>(std::move(f));

        GET_EXPECTED_ASYNC(input, file_mananger->download_stream(std::move(p.file_blocks)));
        CHECK_EXPECTED_ASYNC(co_await input->copy_to(stream));
      }
    }
#ifdef _WIN32
    system(filename(update_folder, "autoupdate.exe").local_name().c_str());
#else//_WIN32
    system(filename(update_folder, "autoupdate").local_name().c_str());
#endif// _WIN32

  }

  co_return vds::expected<void>();
}
