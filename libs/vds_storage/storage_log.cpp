/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_log.h"
#include "storage_log_p.h"
#include "process_log_line.h"
#include "log_records.h"
#include "cert.h"
#include "node.h"
#include "endpoint.h"
#include "certificate_authority.h"
#include "certificate_authority_p.h"

vds::storage_log::storage_log(
  const service_provider & sp,
  const std::string & current_server_id)
  : impl_(new _storage_log(sp, current_server_id, this))
{
}

vds::storage_log::~storage_log()
{
}

void vds::storage_log::reset(
  const std::string & root_password,
  const std::string & addresses)
{
  this->impl_->reset(root_password, addresses);
}

void vds::storage_log::start()
{
  this->impl_->start();
}

void vds::storage_log::stop()
{
  this->impl_->stop();
}

bool vds::storage_log::is_empty() const
{
  return this->impl_->is_empty();
}

vds::_storage_log * vds::storage_log::operator->() const
{
  return this->impl_.get();
}

size_t vds::storage_log::minimal_consensus() const
{
  return this->impl_->minimal_consensus();
}

void vds::storage_log::add_record(const std::string & record)
{
  return this->impl_->add_record(record);
}

size_t vds::storage_log::new_message_id()
{
  return this->impl_->new_message_id();
}

const std::list<vds::endpoint>& vds::storage_log::get_endpoints() const
{
  return this->impl_->get_endpoints();
}

void vds::storage_log::register_server(const std::string & server_certificate)
{
  this->impl_->register_server(server_certificate);
}

///////////////////////////////////////////////////////////////////////////////
vds::_storage_log::_storage_log(
  const service_provider & sp,
  const std::string & current_server_id,
  storage_log * owner)
: current_server_id_(current_server_id),
  owner_(owner),
  log_(sp, "Server log"),
  vds_folder_(persistence::current_user(sp), ".vds"),
  local_log_folder_(foldername(persistence::current_user(sp), ".vds"), "local_log"),
  local_log_index_(0),
  is_empty_(true),
  minimal_consensus_(0),
  last_message_id_(0),
  chunk_storage_(guid::new_guid(), 1000),
  chunk_manager_(sp)
{
}

void vds::_storage_log::reset(
  const std::string & password,
  const std::string & addresses
)
{
  this->local_log_folder_.create();

  this->log_.info("Creating certificate");
  
  asymmetric_private_key private_key(asymmetric_crypto::rsa4096());
  private_key.generate();
  
  certificate root_certificate = _certificate_authority::create_root_user(private_key);

  this->log_.info("Creating server certificate");
  asymmetric_private_key server_private_key(asymmetric_crypto::rsa4096());
  server_private_key.generate();
  
  certificate server_certificate = certificate_authority::create_server(root_certificate, private_key, server_private_key);
  
  this->vds_folder_.create();
  server_certificate.save(filename(this->vds_folder_, "server.crt"));
  server_private_key.save(filename(this->vds_folder_, "server.pkey"));
  
  auto  user_cert_id = this->save_object(
    file_container()
      .add("c", root_certificate.str())
      .add("k", private_key.str(password)));

  hash ph(hash::sha256());
  ph.update(password.c_str(), password.length());
  ph.final();

  this->add_to_local_log(
    server_log_root_certificate(
      user_cert_id,
      base64::from_bytes(ph.signature(), ph.signature_length())).serialize().get());

  auto  sert_cert_id = this->save_object(
    file_container()
    .add("c", server_certificate.str()));

  this->add_to_local_log(server_log_new_server(sert_cert_id).serialize().get());
  this->add_to_local_log(server_log_new_endpoint(addresses).serialize().get());
}


void vds::_storage_log::start()
{
  std::map<uint64_t, filename> local_records;
  this->local_log_folder_.files(
    [&local_records](const filename & fn) -> bool {
    uint64_t index = std::atoll(fn.name().c_str());
    if (std::to_string(index) == fn.name()) {
      local_records[index] = fn;
    }
    return true;
  });

  for (auto & p : local_records) {

    sequence(
      read_file(p.second),
      json_parser("Record " + std::to_string(p.first))
    )(
      [this](json_value * record) {
      this->apply_record(this->current_server_id_, record);
      delete record;
    },
      [](std::exception * ex) {
      throw ex;
    });

    if (this->local_log_index_ < p.first) {
      this->local_log_index_ = p.first + 1;
    }
  }
  /*
  filename fn(this->local_log_folder_, "server_log.json");
  if (fn.exists()) {
    json_parser::options parser_options;
    parser_options.enable_multi_root_objects = true;

    sequence(
      read_file(fn),
      json_parser(fn.name(), parser_options),
      process_log_line<_storage_log>(fn.name(), this)
    )(
      []() {},
      [](std::exception * ex) { throw ex; }
    );
  }
  */
}

void vds::_storage_log::stop()
{
}


bool vds::_storage_log::is_empty()
{
  return this->is_empty_;
}

vds::certificate * vds::_storage_log::get_cert(const std::string & subject)
{
  auto p = this->loaded_certificates_.find(subject);
  if (this->loaded_certificates_.end() == p) {
    return nullptr;
  }

  return p->second.get();
}

vds::certificate * vds::_storage_log::parse_root_cert(const json_value * value)
{
  server_log_root_certificate message(value);
  
  //std::string cert_body;
  //if (message.certificate().empty() || message.private_key().empty()) {
  //  return nullptr;
  //}

  //return new certificate(certificate::parse(message.certificate()));
  return nullptr;
}

void vds::_storage_log::apply_record(const std::string & source_server_id, const json_value * value)
{
  if(this->is_empty_){
    //already processed
    this->is_empty_ = false;
  }

  auto value_obj = dynamic_cast<const json_object *>(value);
  if (nullptr != value_obj) {
    std::string record_type;
    if (value_obj->get_property("$t", record_type, false) && !record_type.empty()) {
      if (server_log_root_certificate::message_type == record_type) {
        this->process(source_server_id, server_log_root_certificate(value_obj));
      }
      else if (server_log_new_server::message_type == record_type) {
        this->process(source_server_id, server_log_new_server(value_obj));
      }
      else if (server_log_new_endpoint::message_type == record_type) {
        this->process(source_server_id, server_log_new_endpoint(value_obj));
      }
      else {
        this->log_(log_level::ll_warning, "Invalid server log record type %s", record_type.c_str());
      }
    }
    else {
      this->log_(log_level::ll_warning, "Invalid server log record: the record has not type attribute");
    }
  }
  else {
    this->log_(log_level::ll_warning, "Invalid server log record: the record in not object");
  }
}

void vds::_storage_log::process(const std::string & source_server_id, const server_log_root_certificate & message)
{
  //auto cert = new certificate(certificate::parse(message.certificate()));
  //this->certificate_store_.add(*cert);
  //this->loaded_certificates_[cert->subject()].reset(cert);

  this->certificates_.push_back(vds::cert("login:root", source_server_id,  message.user_cert(), message.password_hash()));
}

void vds::_storage_log::process(const std::string & source_server_id, const server_log_new_server & message)
{
  //auto cert = new certificate(certificate::parse(message.certificate()));
  //auto result = this->certificate_store_.verify(*cert);

  //if (result.error_code != 0) {
  //  throw new std::runtime_error("Invalid certificate");
  //}

  //this->certificate_store_.add(*cert);
  //this->loaded_certificates_[cert->subject()].reset(cert);

  //this->nodes_.push_back(node(cert->subject(), message.certificate()));
  //this->log_(ll_trace, "add node %s", cert->subject().c_str());
}

void vds::_storage_log::process(const std::string & source_server_id, const server_log_new_endpoint & message)
{
  this->endpoints_.push_back(message.addresses());
}

void vds::_storage_log::add_record(const std::string & record)
{
  //sequence(
  //  json_parser("Record"),
  //  process_log_line<_storage_log>("Record", this)
  //)(
  //  [this, &record]() {
  //  file f(filename(this->commited_folder_, "checkpoint0.json").local_name(), file::append);
  //  output_text_stream os(f);
  //  os.write(record);
  //  os.write("\n");
  //},
  //  [](std::exception * ex) { throw ex; },
  //  record.c_str(),
  //  record.length()
  //);
}

size_t vds::_storage_log::new_message_id()
{
  return this->last_message_id_++;
}

void vds::_storage_log::register_server(const std::string & server_certificate)
{
  auto id = this->save_object(file_container().add("c", server_certificate));

  this->add_to_local_log(server_log_new_server(id).serialize().get());
}


uint64_t vds::_storage_log::save_object(const file_container & fc)
{
  binary_serializer s;
  fc.serialize(s);

  return this->chunk_manager_.add(s.data());
}

void vds::_storage_log::add_to_local_log(const json_value * record)
{
  std::lock_guard<std::mutex> lock(this->local_log_mutex_);

  file f(filename(this->local_log_folder_, std::to_string(this->local_log_index_++)).local_name(), file::create_new);
  output_text_stream os(f);
  os.write(record->str());
}

