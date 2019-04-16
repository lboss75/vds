/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vds_updater_app.h"

int main(int argc, const char **argv)
{
  vds::updater::vds_updater_app app;
  return app.run(argc, argv);
}

vds::updater::vds_updater_app::vds_updater_app()
: update_command_set_("Update", "Update application", "update") {

}

vds::expected<void> vds::updater::vds_updater_app::main(const service_provider* sp) {
  if(this->current_command_set_ == &this->update_command_set_) {
#ifdef _WIN32
    HWND hWnd = FindWindow(_T("VDS Windows Class"), NULL);
    if(NULL != hWnd) {
      SendMessage(hWnd, WM_QUIT, 0, 0);
    }
#else//_WIN32
#endif
    GET_EXPECTED(current_user_folder, persistence::current_user(sp));

    foldername target_folder(current_user_folder, "app");
    foldername backup_folder(current_user_folder, "app.bak");

    CHECK_EXPECTED(backup_folder.delete_folder(true));
    CHECK_EXPECTED(backup_folder.create());

    std::map<filename, filename> moved_files;
    
    auto result = target_folder.files([&moved_files, &backup_folder](const filename & fn) -> expected<bool>{
      filename target_file(backup_folder, fn.name());
      CHECK_EXPECTED(file::move(fn, target_file));
      moved_files.emplace(fn, target_file);
      return true;
    });

    if(!result.has_error()) {
      auto this_folder = this->current_process_.contains_folder();
      result = this_folder.files([&target_folder](const filename & fn) -> expected<bool> {
        filename target_file(target_folder, fn.name());
        CHECK_EXPECTED(file::copy(fn, target_file, true));
        return true;
      });

      if(!result.has_error()) {
#ifdef _WIN32
        system(foldername(target_folder, "vds_windows.exe").local_name().c_str());
#else//_WIN32
        system(foldername(target_folder, "vds_web_server").local_name().c_str());
#endif//_WIN32
      }
    }
    //Try to restore files
    for (auto & p : moved_files) {
      if(file::exists(p.first)) {
        (void)file::delete_file(p.first);
      }
      (void)file::move(p.second, p.first);
    }

    return std::move(result);
  }

  return expected<void>();
}

void vds::updater::vds_updater_app::register_command_line(command_line& cmd_line) {
  cmd_line.add_command_set(this->update_command_set_);
}
