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

vds::storage_log::storage_log(const service_provider & sp)
  : impl_(new _storage_log(sp, this))
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

///////////////////////////////////////////////////////////////////////////////
vds::_storage_log::_storage_log(const service_provider & sp, storage_log * owner)
: owner_(owner),
  log_(sp, "Server log"),
  vds_folder_(persistence::current_user(sp), ".vds"),
  commited_folder_(foldername(persistence::current_user(sp), ".vds"), "commited"),
  is_empty_(true),
  minimal_consensus_(0),
  last_message_id_(0)
{
}

void vds::_storage_log::reset(
  const std::string & password,
  const std::string & addresses
)
{
  asymmetric_private_key private_key(asymmetric_crypto::rsa4096());
  private_key.generate();

  asymmetric_public_key pkey(private_key);

  this->log_.info("Creating certificate");
  certificate::create_options options;
  options.country = "RU";
  options.organization = "IVySoft";
  options.name = "Certificate " + guid::new_guid().str();

  certificate root_certificate = certificate::create_new(pkey, private_key, options);

  hash ph(hash::sha256());
  ph.update(password.c_str(), password.length());
  ph.final();

  asymmetric_private_key server_private_key(asymmetric_crypto::rsa4096());
  server_private_key.generate();

  asymmetric_public_key server_pkey(server_private_key);

  this->log_.info("Creating server certificate");
  certificate::create_options server_options;
  server_options.country = "RU";
  server_options.organization = "IVySoft";
  server_options.name = "Certificate " + guid::new_guid().str();
  server_options.ca_certificate = &root_certificate;
  server_options.ca_certificate_private_key = &private_key;

  certificate server_certificate = certificate::create_new(server_pkey, server_private_key, server_options);
  this->vds_folder_.create();
  server_certificate.save(filename(this->vds_folder_, "server.crt"));
  server_private_key.save(filename(this->vds_folder_, "server.pkey"));

  std::unique_ptr<server_log_batch> batch(new server_log_batch((size_t)0u));

  batch->add(
    server_log_root_certificate(
      root_certificate.str(),
      private_key.str(password),
      base64::from_bytes(ph.signature(), ph.signature_length())).serialize());

  batch->add(server_log_new_server(server_certificate.str()).serialize());
  batch->add(server_log_new_endpoint(addresses).serialize());

  auto message_body = batch->serialize()->str();

  hash h(hash::sha256());
  h.update(message_body.c_str(), message_body.length());
  h.final();

  asymmetric_sign s(hash::sha256(), private_key);
  s.update(h.signature(), h.signature_length());
  s.final();

  this->commited_folder_.create();
  
  server_log_record record(std::move(batch));
  record.add_signature(root_certificate.subject(), s.signature());
  
  file f(filename(this->commited_folder_, "checkpoint0.json").local_name(), file::truncate);
  output_text_stream os(f);
  os.write(record.serialize(false)->str());
  os.write("\n");
}


void vds::_storage_log::start()
{
  filename fn(this->commited_folder_, "checkpoint0.json");
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
  
  std::string cert_body;
  if (message.certificate().empty() || message.private_key().empty()) {
    return nullptr;
  }

  return new certificate(certificate::parse(message.certificate()));
}

void vds::_storage_log::apply_record(const json_value * value)
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
        this->process(server_log_root_certificate(value_obj));
      }
      else if (server_log_new_server::message_type == record_type) {
        this->process(server_log_new_server(value_obj));
      }
      else if (server_log_new_endpoint::message_type == record_type) {
        this->process(server_log_new_endpoint(value_obj));
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

void vds::_storage_log::process(const server_log_root_certificate & message)
{
  auto cert = new certificate(certificate::parse(message.certificate()));
  this->certificate_store_.add(*cert);
  this->loaded_certificates_[cert->subject()].reset(cert);

  this->certificates_.push_back(vds::cert("login:root", message.certificate(), message.private_key(), message.password_hash()));
}

void vds::_storage_log::process(const server_log_new_server & message)
{
  auto cert = new certificate(certificate::parse(message.certificate()));
  auto result = this->certificate_store_.verify(*cert);

  if (result.error_code != 0) {
    throw new std::runtime_error("Invalid certificate");
  }

  this->certificate_store_.add(*cert);
  this->loaded_certificates_[cert->subject()].reset(cert);

  this->nodes_.push_back(node(cert->subject(), message.certificate()));
  this->log_(ll_trace, "add node %s", cert->subject().c_str());
}

void vds::_storage_log::process(const server_log_new_endpoint & message)
{
  this->endpoints_.push_back(message.addresses());
}

void vds::_storage_log::add_record(const std::string & record)
{
  sequence(
    json_parser("Record"),
    process_log_line<_storage_log>("Record", this)
  )(
    [this, &record]() {
    file f(filename(this->commited_folder_, "checkpoint0.json").local_name(), file::append);
    output_text_stream os(f);
    os.write(record);
    os.write("\n");
  },
    [](std::exception * ex) { throw ex; },
    record.c_str(),
    record.length()
  );
}

size_t vds::_storage_log::new_message_id()
{
  return this->last_message_id_++;
}
