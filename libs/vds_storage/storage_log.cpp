/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_log.h"
#include "process_log_line.h"

vds::storage_log::storage_log()
  : commited_folder_(foldername(persistence::current_user(), ".vds"), "commited"),
  is_empty_(true)
{
}

void vds::storage_log::reset(
  const certificate & root_certificate,
  const asymmetric_private_key & private_key,
  const std::string & password
)
{
  std::unique_ptr<json_object> m(new json_object());
  m->add_property("$i", "0");
  m->add_property("$t", "certificate");
  m->add_property("c", root_certificate.str());
  m->add_property("k", private_key.str(password));

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

  file f(filename(this->commited_folder_, "checkpoint0.json").local_name(), file::truncate);
  output_text_stream os(f);
  os.write("{\"f\":\"");
  os.write(root_certificate.fingerprint());
  os.write("\",\"s\":\"");
  os.write(base64::from_bytes(s.signature(), s.signature_length()));
  os.write("\",\"m\":");
  os.write(message_body);
  os.write("}\n");
}

void vds::storage_log::start()
{
  filename fn(this->commited_folder_, "checkpoint0.json");
  
  json_parser::options parser_options;
  parser_options.enable_multi_root_objects = true;

  auto done_handler = lambda_handler([]() {});
  auto error_handler = lambda_handler([](std::exception * ex) { throw ex; });
  
  sequence(
    read_file(fn),
    json_parser(fn.name(), parser_options),
    process_log_line<storage_log>(this)
  )(
    done_handler,
    error_handler
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
  auto cert_obj = dynamic_cast<const json_object *>(value);
  if (nullptr == cert_obj) {
    return nullptr;
  }

  std::string cert_body;
  if (!cert_obj->get_property_string("c", cert_body)) {
    return nullptr;
  }

  auto result = new certificate(certificate::parse(cert_body));
  this->certificates_[result->fingerprint()].reset(result);
  return result;
}

void vds::storage_log::apply_record(const json_value * value)
{
  if(this->is_empty_){
    //already processed
    this->is_empty_ = false;
  }
}
