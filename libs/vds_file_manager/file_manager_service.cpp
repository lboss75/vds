//
// Created by vadim on 02.01.18.
//

#include "file_manager_service.h"
#include "file_operations.h"
#include "private/file_manager_service_p.h"

vds::file_manager::file_manager_service::file_manager_service()
    : impl_(new file_manager_private::_file_manager_service()){
}

vds::file_manager::file_manager_service::~file_manager_service(){
  delete this->impl_;
}

void vds::file_manager::file_manager_service::register_services(vds::service_registrator & registrator) {
  this->impl_->register_services(registrator);
}

void vds::file_manager::file_manager_service::start(const service_provider * sp) {
  this->impl_->start(sp);
}

void vds::file_manager::file_manager_service::stop() {
  this->impl_->stop();
}

vds::async_task<void> vds::file_manager::file_manager_service::prepare_to_stop() {
  return this->impl_->prepare_to_stop();
}

/////////////////////////////////
void vds::file_manager_private::_file_manager_service::register_services(vds::service_registrator & registrator) {
  registrator.add_service<file_manager::file_operations>(&this->file_operations_);
}

void vds::file_manager_private::_file_manager_service::start(const service_provider * sp) {
  this->file_operations_.start(sp);
}

void vds::file_manager_private::_file_manager_service::stop() {
  this->file_operations_.stop();
}

vds::async_task<void> vds::file_manager_private::_file_manager_service::prepare_to_stop() {
  return this->file_operations_.prepare_to_stop();
}

