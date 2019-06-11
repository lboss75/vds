/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "get_root_app.h"
#include "keys_control.h"

vds::get_root_app::get_root_app()
  : key_generate_command_set_("Generate keys", "Generate keys", "generate", "keys"),
  user_login_(
    "l",
    "login",
    "Login",
    "User login"),
  user_password_(
    "p",
    "password",
    "Password",
    "User password")
{
}

void out_multiline(const std::string & value, std::size_t width = 80) {
  for(std::size_t pos = 0; pos < value.length();) {
    if(value.length() > pos + width) {
      std::cout << "\"" << value.substr(pos, width) << "\"\n";
      pos += width;
    }
    else {
      std::cout << "\"" << value.substr(pos) << "\"";
      break;
    }
  }
}
#define write_member(member_name)\
  std::cout << "char vds::keys_control::" #member_name "[" \
  << sizeof(keys_control::member_name) \
  << "] = \n";\
  out_multiline(keys_control::member_name);\
  std::cout << ";\n";

vds::expected<void> vds::get_root_app::main(const service_provider * /*sp*/)
{
  if (&this->key_generate_command_set_ == this->current_command_set_) {
    keys_control::private_info_t private_info;
    CHECK_EXPECTED(private_info.genereate_all());

    CHECK_EXPECTED(keys_control::genereate_all(
        private_info));
    
    write_member(common_news_channel_id_);
    write_member(common_news_read_public_key_);
    write_member(common_news_read_private_key_);
    write_member(common_news_write_public_key_);
    write_member(common_news_admin_public_key_);

    write_member(autoupdate_channel_id_);
    write_member(autoupdate_read_public_key_);
    write_member(autoupdate_read_private_key_);
    write_member(autoupdate_write_public_key_);
    write_member(autoupdate_admin_public_key_);

    write_member(web_channel_id_);
    write_member(web_read_public_key_);
    write_member(web_read_private_key_);
    write_member(web_write_public_key_);
    write_member(web_admin_public_key_);

    binary_serializer s;
    CHECK_EXPECTED(s << private_info.common_news_write_private_key_->der(this->user_password_.value()));
    CHECK_EXPECTED(s << private_info.common_news_admin_private_key_->der(this->user_password_.value()));

    CHECK_EXPECTED(s << private_info.autoupdate_write_private_key_->der(this->user_password_.value()));
    CHECK_EXPECTED(s << private_info.autoupdate_admin_private_key_->der(this->user_password_.value()));

    CHECK_EXPECTED(s << private_info.web_write_private_key_->der(this->user_password_.value()));
    CHECK_EXPECTED(s << private_info.web_admin_private_key_->der(this->user_password_.value()));

    CHECK_EXPECTED(file::write_all(filename("keys"), s.move_data()));
  }

  return expected<void>();
}

void vds::get_root_app::register_services(vds::service_registrator& registrator)
{
  base_class::register_services(registrator);
  registrator.add(this->mt_service_);
  registrator.add(this->task_manager_);
  registrator.add(this->crypto_service_);
}

void vds::get_root_app::register_command_line(command_line & cmd_line)
{
  base_class::register_command_line(cmd_line);

  cmd_line.add_command_set(this->key_generate_command_set_);
  this->key_generate_command_set_.required(this->user_login_);
  this->key_generate_command_set_.required(this->user_password_);
}

vds::expected<void> vds::get_root_app::start_services(service_registrator & registrator, service_provider * sp)
{
  return base_class::start_services(registrator, sp);
}
