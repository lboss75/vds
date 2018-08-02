/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "get_root_app.h"
#include "cert_control.h"

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

void vds::get_root_app::main(const service_provider & sp)
{
  if (&this->key_generate_command_set_ == this->current_command_set_) {
    vds::imt_service::enable_async(sp);

    cert_control::genereate_all(
        this->user_login_.value(),
      this->user_password_.value());
    
    std::cout << "char vds::cert_control::root_certificate_["
      << (strlen(cert_control::root_certificate_) + 1)
      << "] = ";
    out_multiline(cert_control::root_certificate_);
    std::cout << ";\n";

    std::cout << "char vds::cert_control::root_private_key_["
      << (strlen(cert_control::root_private_key_) + 1)
      << "] = ";
    out_multiline(cert_control::root_private_key_);
    std::cout << ";\n";
  }
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

void vds::get_root_app::start_services(service_registrator & registrator, service_provider & sp)
{
  base_class::start_services(registrator, sp);
}
