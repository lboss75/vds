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
  const guid & current_server_id,
  const certificate & server_certificate,
  const asymmetric_private_key & server_private_key)
  : impl_(new _storage_log(sp,
    current_server_id,
    server_certificate,
    server_private_key,
    this))
{
}

vds::storage_log::~storage_log()
{
}

void vds::storage_log::reset(
  const vds::certificate & root_certificate,
  const asymmetric_private_key & private_key,
  const std::string & root_password,
  const std::string & addresses)
{
  this->impl_->reset(root_certificate, private_key, root_password, addresses);
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

void vds::storage_log::register_server(const std::string & server_certificate)
{
  this->impl_->register_server(server_certificate);
}

std::unique_ptr<vds::cert> vds::storage_log::find_cert(const std::string & object_name) const
{
  return this->impl_->find_cert(object_name);
}

std::unique_ptr<vds::data_buffer> vds::storage_log::get_object(const vds::full_storage_object_id& object_id)
{
  return this->impl_->get_object(object_id);
}

void vds::storage_log::add_endpoint(
  const std::string & endpoint_id,
  const std::string & addresses)
{
  this->impl_->add_endpoint(endpoint_id, addresses);
}

void vds::storage_log::get_endpoints(std::map<std::string, std::string> & addresses)
{
  this->impl_->get_endpoints(addresses);
}

///////////////////////////////////////////////////////////////////////////////
vds::_storage_log::_storage_log(
  const service_provider & sp,
  const guid & current_server_id,
  const certificate & server_certificate,
  const asymmetric_private_key & server_private_key,
  storage_log * owner)
: db_(sp),
  local_cache_(sp),
  server_certificate_(server_certificate),
  current_server_key_(server_private_key),
  current_server_id_(current_server_id),
  owner_(owner),
  log_(sp, "Server log"),
  vds_folder_(persistence::current_user(sp), ".vds"),
  local_log_folder_(foldername(persistence::current_user(sp), ".vds"), "local_log"),
  local_log_index_(0),
  is_empty_(true),
  minimal_consensus_(0),
  last_message_id_(0),
  chunk_storage_(guid::new_guid(), 1000),
  chunk_manager_(sp, current_server_id, local_cache_)
{
}

void vds::_storage_log::reset(
  const certificate & root_certificate,
  const asymmetric_private_key & private_key,
  const std::string & password,
  const std::string & addresses
)
{
  this->db_.start();

  this->local_log_folder_.create();
  this->vds_folder_.create();
  
  this->server_certificate_.save(filename(this->vds_folder_, "server.crt"));
  this->current_server_key_.save(filename(this->vds_folder_, "server.pkey"));
  
  auto  user_cert_id = this->save_object(
    object_container()
      .add("c", root_certificate.str())
      .add("k", private_key.str(password)));
  
  hash ph(hash::sha256());
  ph.update(password.c_str(), password.length());
  ph.final();

  this->add_to_local_log(
    server_log_root_certificate(
      user_cert_id,
      ph.signature()).serialize().get());

  auto  sert_cert_id = this->save_object(
    object_container()
    .add("c", this->server_certificate_.str()));

  this->add_to_local_log(server_log_new_server(sert_cert_id).serialize().get());
  this->add_to_local_log(server_log_new_endpoint(this->current_server_id_, addresses).serialize().get());
  
  this->db_.add_cert(
    cert(
      "login::root",
      full_storage_object_id(this->current_server_id_, user_cert_id),
      ph.signature()));
}


void vds::_storage_log::start()
{
  this->db_.start();
  this->chunk_manager_.set_next_index(
    this->db_.last_object_index(this->current_server_id_));

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
  return nullptr;
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

void vds::_storage_log::apply_record(const guid & source_server_id, const json_value * value)
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

void vds::_storage_log::process(const guid & source_server_id, const server_log_root_certificate & message)
{
  //auto cert = new certificate(certificate::parse(message.certificate()));
  //this->certificate_store_.add(*cert);
  //this->loaded_certificates_[cert->subject()].reset(cert);

  this->db_.add_cert(
    vds::cert(
      "login:root",
      full_storage_object_id(
        source_server_id,
        message.user_cert()),
      message.password_hash()));
}

void vds::_storage_log::process(const guid & source_server_id, const server_log_new_server & message)
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

void vds::_storage_log::process(const guid & source_server_id, const server_log_new_endpoint & message)
{
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
  auto id = this->save_object(object_container().add("c", server_certificate));

  this->add_to_local_log(server_log_new_server(id).serialize().get());
}

vds::storage_object_id vds::_storage_log::save_object(const object_container & fc)
{
  binary_serializer s;
  fc.serialize(s);

  asymmetric_sign sign(hash::sha256(), this->current_server_key_);
  sign.update(s.data().data(), s.data().size());
  sign.final();

  auto index = this->chunk_manager_.add(s.data());
  auto signature = sign.signature();
  this->db_.add_object(this->current_server_id_, index, signature);
  return vds::storage_object_id(index, signature);
}

void vds::_storage_log::add_to_local_log(const json_value * record)
{
  std::lock_guard<std::mutex> lock(this->local_log_mutex_);

  file f(filename(this->local_log_folder_, std::to_string(this->local_log_index_++)).local_name(), file::create_new);
  output_text_stream os(f);
  os.write(record->str());
}

std::unique_ptr<vds::cert> vds::_storage_log::find_cert(const std::string & object_name) const
{
  return this->db_.find_cert(object_name);
}

std::unique_ptr<vds::data_buffer> vds::_storage_log::get_object(const vds::full_storage_object_id& object_id)
{
  return this->local_cache_.get_object(object_id);
}

void vds::_storage_log::add_endpoint(
  const std::string & endpoint_id,
  const std::string & addresses)
{
  this->db_.add_endpoint(endpoint_id, addresses);
}

void vds::_storage_log::get_endpoints(std::map<std::string, std::string> & addresses)
{
  this->db_.get_endpoints(addresses);
}
