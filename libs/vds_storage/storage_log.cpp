/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_log.h"
#include "process_log_line.h"
#include "log_records.h"

vds::storage_log::storage_log(const service_provider & sp)
: log_(sp, "Server log"),
  vds_folder_(persistence::current_user(sp), ".vds"),
  commited_folder_(foldername(persistence::current_user(sp), ".vds"), "commited"),
  is_empty_(true)
{
}

void vds::storage_log::reset(
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
  options.name = "Root Certificate";

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
  server_options.name = "Root Certificate";
  server_options.ca_certificate = &root_certificate;
  server_options.ca_certificate_private_key = &private_key;

  certificate server_certificate = certificate::create_new(server_pkey, server_private_key, server_options);
  this->vds_folder_.create();
  server_certificate.save(filename(this->vds_folder_, "server.crt"));
  server_private_key.save(filename(this->vds_folder_, "server.pkey"));


  server_log_new_server new_server_message;
  new_server_message.certificate_ = server_certificate.str();
  new_server_message.addresses_ = addresses;

  server_log_batch batch;
  batch.message_id_ = 0;
  batch.previous_message_id_ = 0;
  batch.messages_.reset(new json_array());

  batch.messages_->add(
    server_log_root_certificate(
      root_certificate.str(),
      private_key.str(password),
      base64::from_bytes(ph.signature(), ph.signature_length())).serialize());
  batch.messages_->add(new_server_message.serialize().release());
   
  std::unique_ptr<json_value> m(batch.serialize());

  json_writer writer;
  m->str(writer);

  auto message_body = writer.str();

  hash h(hash::sha256());
  h.update(message_body.c_str(), message_body.length());
  h.final();

  asymmetric_sign s(hash::sha256(), private_key);
  s.update(h.signature(), h.signature_length());
  s.final();

  this->commited_folder_.create();
  
  server_log_record record;
  record.fingerprint_ = root_certificate.fingerprint();
  record.signature_ = base64::from_bytes(s.signature(), s.signature_length());
  record.message_ = std::move(m);
  
  json_writer record_writer;
  record.serialize()->str(record_writer);

  file f(filename(this->commited_folder_, "checkpoint0.json").local_name(), file::truncate);
  output_text_stream os(f);
  os.write(record_writer.str());
  os.write("\n");
}


void vds::storage_log::start()
{
  filename fn(this->commited_folder_, "checkpoint0.json");
  
  json_parser::options parser_options;
  parser_options.enable_multi_root_objects = true;
  
  sequence(
    read_file(fn),
    json_parser(fn.name(), parser_options),
    process_log_line<storage_log>(fn.name(), this)
  )(
    []() {},
    [](std::exception * ex) { throw ex; }
  );
}

bool vds::storage_log::is_empty()
{
  return this->is_empty_;
}

vds::certificate * vds::storage_log::get_cert(const std::string & fingerprint)
{
  auto p = this->certificates_.find(fingerprint);
  if (this->certificates_.end() == p) {
    return nullptr;
  }

  return p->second.get();
}

vds::certificate * vds::storage_log::parse_root_cert(const json_value * value)
{
  server_log_root_certificate message(value);
  
  std::string cert_body;
  if (message.certificate().empty() || message.private_key().empty()) {
    return nullptr;
  }

  return new certificate(certificate::parse(message.certificate()));
}

void vds::storage_log::apply_record(const json_value * value)
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

bool vds::storage_log::get_cert_and_key(const std::string & object_name, const std::string & password_hash, std::string & cert_body, std::string & key_body)
{
  auto p = this->cert_and_keys_.find(object_name);
  if (this->cert_and_keys_.end() == p) {
    return false;
  }

  auto p1 = p->second.find(password_hash);
  if (p->second.end() == p1) {
    return false;
  }

  cert_body = p1->second.cert_body;
  key_body = p1->second.key_body;
  return true;
}

void vds::storage_log::process(const server_log_root_certificate & message)
{
  auto cert = new certificate(certificate::parse(message.certificate()));
  this->certificates_[cert->fingerprint()].reset(cert);

  this->cert_and_keys_["login:root"][message.password_hash()] = { message.certificate(), message.private_key() };
}
