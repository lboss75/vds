#include <transactions/channel_create_transaction.h>
#include <transactions/device_user_add_transaction.h>
#include "stdafx.h"
#include "security_walker.h"
#include "transactions/channel_add_writer_transaction.h"
#include "vds_debug.h"
#include "certificate_chain_dbo.h"
#include "transactions/channel_add_reader_transaction.h"
#include "transaction_block.h"

vds::security_walker::security_walker(
    const const_data_buffer & dht_user_id,
    const symmetric_key & user_password_key)
: dht_user_id_(dht_user_id),
  user_password_key_(user_password_key){
}

void vds::security_walker::load(
  const service_provider & parent_scope,
  database_transaction &t) {
  auto sp = parent_scope.create_scope(__FUNCTION__);
  const auto log = sp.get<logger>();
  log->trace(ThisModule, sp, "security_walker::load");

//  this->certificate_chain_[cert_control::get_id(this->user_cert_)] = this->user_cert_;
//
//  orm::certificate_chain_dbo t2;
//  auto cert_id = cert_control::get_parent_id(this->user_cert_);
//  while (cert_id) {
//
//    auto st = t.get_reader(t2.select(t2.cert).where(t2.id == cert_id));
//    if (!st.execute()) {
//      throw std::runtime_error("Wrong certificate id " + cert_id.str());
//    }
//    const auto cert = certificate::parse_der(t2.cert.get(st));
//    log->debug(ThisModule, sp, "Loaded certificate %s",
//      cert_id.str().c_str());
//
//    this->certificate_chain_[cert_id] = cert;
//    cert_id = cert_control::get_parent_id(cert);
//  }

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1.select(t1.data)
    .where(t1.channel_id == base64::from_bytes(this->dht_user_id_))
    .order_by(t1.order_no));

  while (st.execute()) {
    guid channel_id;
    uint64_t order_no;
    guid read_cert_id;
    guid write_cert_id;
    std::set<const_data_buffer> ancestors;
    const_data_buffer crypted_data;
    const_data_buffer crypted_key;
    const_data_buffer signature;

    transactions::transaction_block::parse_block(
        t1.data.get(st),
        channel_id,
        order_no,
        read_cert_id,
        write_cert_id,
        ancestors,
        crypted_data,
        crypted_key,
        signature);

    if (channel_id != this->user_id_
      || read_cert_id != cert_control::get_id(this->user_cert_)
      || write_cert_id != cert_control::get_id(this->user_cert_)) {
      throw std::runtime_error("Ivalid record");
    }

    if (!transactions::transaction_block::validate_block(
        this->user_cert_,
        channel_id,
        order_no,
        read_cert_id,
        write_cert_id,
        ancestors,
        crypted_data,
        crypted_key,
        signature)) {
      log->error(ThisModule, sp, "Write signature error");
      throw std::runtime_error("Write signature error");
    }

    const auto key_data = this->user_private_key_.decrypt(crypted_key);
    const auto key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data);
    const auto data = symmetric_decrypt::decrypt(key, crypted_data);

    binary_deserializer s(data);

    while(0 < s.size()) {
      uint8_t message_id;
      s >> message_id;

      switch ((transactions::transaction_id) message_id) {
        case transactions::transaction_id::channel_add_reader_transaction: {
          transactions::channel_add_reader_transaction message(s);
          auto &cp = this->channels_[message.channel_id()];

          cp.name_ = message.name();

          auto write_id = cert_control::get_id(message.write_cert());
          cp.write_certificates_[write_id] = message.write_cert();

          auto id = cert_control::get_id(message.read_cert());
          cp.read_certificates_[id] = message.read_cert();
          cp.read_private_keys_[id] = message.read_private_key();
          cp.current_read_certificate_ = id;

          log->debug(ThisModule, sp, "Got channel %s reader certificate %s",
                     base64::from_bytes(message.channel_id()).c_str(),
                     id.str().c_str());

          break;
        }

        case transactions::transaction_id::channel_add_writer_transaction: {
          transactions::channel_add_writer_transaction message(s);
          auto &cp = this->channels_[message.channel_id()];
          auto id = cert_control::get_id(message.write_cert());
          cp.name_ = message.name();
          cp.write_certificates_[id] = message.write_cert();
          cp.write_private_keys_[id] = message.write_private_key();
          cp.current_write_certificate_ = id;

          auto read_id = cert_control::get_id(message.read_cert());
          cp.read_certificates_[read_id] = message.read_cert();
          cp.current_read_certificate_ = read_id;

          log->debug(ThisModule, sp, "Got channel %s write certificate %s, read certificate %s",
                     base64::from_bytes(message.channel_id()).c_str(),
                     id.str().c_str(),
                     read_id.str().c_str());

          break;
        }

        case transactions::transaction_id::channel_create_transaction: {
          transactions::channel_create_transaction message(s);
          auto &cp = this->channels_[message.channel_id()];
          cp.name_ = message.name();
          cp.current_read_certificate_ = cert_control::get_id(message.read_cert());
          cp.current_write_certificate_ = cert_control::get_id(message.write_cert());
          cp.read_certificates_[cp.current_read_certificate_] = message.read_cert();
          cp.read_private_keys_[cp.current_read_certificate_] = message.read_private_key();
          cp.write_certificates_[cp.current_write_certificate_] = message.write_cert();
          cp.write_private_keys_[cp.current_write_certificate_] = message.write_private_key();

          log->debug(ThisModule, sp, "New channel %s(%s), read certificate %s, write certificate %s",
                     message.channel_id().str().c_str(),
                     message.name().c_str(),
                     cp.current_read_certificate_.str().c_str(),
                     cp.current_write_certificate_.str().c_str());

          break;
        }

        case transactions::transaction_id::device_user_add_transaction: {
          transactions::device_user_add_transaction message(s);
          break;
        }

        default:
          throw std::runtime_error("logic error");
      }
    }
  }
}

bool
vds::security_walker::get_channel_write_certificate(
    const guid &channel_id,
    std::string & name,
	certificate & read_certificate,
	asymmetric_private_key &read_key,
    certificate & write_certificate,
    asymmetric_private_key &write_key) const {
  auto p = this->channels_.find(channel_id);
  if(this->channels_.end() == p){
    return false;
  }

  name = p->second.name_;

  if (p->second.current_read_certificate_) {
	  auto p1 = p->second.read_certificates_.find(p->second.current_read_certificate_);
	  if (p->second.read_certificates_.end() != p1) {
		  read_certificate = p1->second;
	  }
	  auto p2 = p->second.read_private_keys_.find(p->second.current_read_certificate_);
	  if (p->second.read_private_keys_.end() != p2) {
		  read_key = p2->second;
	  }
  }

  if (p->second.current_write_certificate_) {
	  auto p1 = p->second.write_certificates_.find(p->second.current_write_certificate_);
	  if (p->second.write_certificates_.end() != p1) {
		  write_certificate = p1->second;
	  }
	  auto p2 = p->second.write_private_keys_.find(p->second.current_write_certificate_);
	  if (p->second.write_private_keys_.end() != p2) {
		  write_key = p2->second;
	  }
  }

  return true;
}

void vds::security_walker::add_certificate(const vds::certificate &cert) {
  this->certificate_chain_[cert_control::get_id(cert)] = cert;
}

