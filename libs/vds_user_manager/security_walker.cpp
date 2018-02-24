#include <transactions/channel_create_transaction.h>
#include "stdafx.h"
#include "security_walker.h"
#include "channel_message.h"
#include "transactions/channel_add_writer_transaction.h"
#include "vds_debug.h"
#include "certificate_chain_dbo.h"
#include "transactions/channel_add_reader_transaction.h"

vds::security_walker::security_walker(
	const guid & common_channel_id,
    const vds::guid &user_id,
    const vds::certificate &user_cert,
    const vds::asymmetric_private_key &user_private_key)
: common_channel_id_(common_channel_id),
  user_id_(user_id),
  user_cert_(user_cert),
  user_private_key_(user_private_key){
	this->certificate_chain_[cert_control::get_id(this->user_cert_)] = this->user_cert_;
}

void vds::security_walker::load(
	const service_provider & parent_scope,
	database_transaction &t) {
	auto sp = parent_scope.create_scope(__FUNCTION__);
	const auto log = sp.get<logger>();
	log->trace(ThisModule, sp, "security_walker::load");

	this->certificate_chain_[cert_control::get_id(this->user_cert_)] = this->user_cert_;

	dbo::certificate_chain_dbo t2;
	auto cert_id = cert_control::get_parent_id(this->user_cert_);
	while (cert_id) {

		auto st = t.get_reader(t2.select(t2.cert).where(t2.id == cert_id));
		if (!st.execute()) {
			throw std::runtime_error("Wrong certificate id " + cert_id.str());
		}
		const auto cert = certificate::parse_der(t2.cert.get(st));
		log->debug(ThisModule, sp, "Loaded certificate %s",
			cert_id.str().c_str());

		this->certificate_chain_[cert_id] = cert;
		cert_id = cert_control::get_parent_id(cert);
	}

  dbo::channel_message t1;
  auto st = t.get_reader(
      t1.select(
              t1.channel_id,
              t1.message_id,
              t1.message,
              t1.read_cert_id,
              t1.write_cert_id,
              t1.signature)
          .where(
             t1.message_id == (int) transactions::transaction_id::channel_add_reader_transaction
          || t1.message_id == (int) transactions::transaction_id::channel_create_transaction
          || t1.message_id == (int) transactions::transaction_id::channel_add_writer_transaction
          || t1.message_id == (int) transactions::transaction_id::channel_remove_reader_transaction)
          .order_by(t1.id));

  while (st.execute()) {
    log->trace(ThisModule, sp, "Apply message %d", t1.message_id.get(st));
    this->apply(
		sp,
        t1.channel_id.get(st),
        t1.message_id.get(st),
        t1.read_cert_id.get(st),
        t1.write_cert_id.get(st),
        t1.message.get(st),
        t1.signature.get(st));
  }
}

vds::const_data_buffer vds::security_walker::decrypt_message(
    const service_provider & parent_scope,
    const guid & channel_id,
    int message_id,
    const guid & read_cert_id,
    const guid & write_cert_id,
    const const_data_buffer & message_data,
    const const_data_buffer & signature) {

  auto sp = parent_scope.create_scope(__FUNCTION__);
  auto log = sp.get<logger>();

  log->debug(ThisModule, sp, "Decrypt channel %s message %d, read cert %s, write_cert %s, message len %d, signature len %d",
	  channel_id.str().c_str(),
	  message_id,
	  read_cert_id.str().c_str(),
	  write_cert_id.str().c_str(),
	  message_data.size(),
	  signature.size());

  auto &cp = this->channels_[channel_id];

  certificate write_cert;
  auto p = cp.write_certificates_.find(write_cert_id);
  if (cp.write_certificates_.end() == p) {
    log->warning(ThisModule, sp, "Write cert %s has not been found",
                 write_cert_id.str().c_str());
    if (channel_id == cert_control::get_user_id(this->user_cert_)) {
      write_cert = this->get_certificate(write_cert_id);
    } else {
      throw std::runtime_error(
          string_format(
              "Unknown write certificate %s",
              write_cert_id.str().c_str()));
    }
  } else {
    write_cert = p->second;
  }

  if (!asymmetric_sign_verify::verify(
      hash::sha256(),
      write_cert.public_key(),
      signature,
      (binary_serializer()
          << (uint8_t) message_id
          << channel_id
          << read_cert_id
          << write_cert_id
          << message_data).data())) {
    log->error(ThisModule, sp, "Write signature error");
    throw std::runtime_error("Write signature error");
  }

  asymmetric_private_key read_key;
  auto p1 = cp.read_private_keys_.find(read_cert_id);
  if (cp.read_private_keys_.end() == p1) {
    log->warning(ThisModule, sp, "Read cert %s has not been found",
                 read_cert_id.str().c_str());
    if (channel_id == cert_control::get_user_id(this->user_cert_)) {
      read_key = this->user_private_key_;
    } else {
      throw std::runtime_error(
          string_format(
              "Unknown read certificate %s private key",
              read_cert_id.str().c_str()));
    }
  } else {
    read_key = p1->second;
  }

  binary_deserializer message_stream(message_data);
  const_data_buffer key_data;
  const_data_buffer crypted_data;

  message_stream >> key_data >> crypted_data;

  auto key_info = read_key.decrypt(key_data);
  auto key = symmetric_key::deserialize(
      symmetric_crypto::aes_256_cbc(),
      binary_deserializer(key_info));

  return symmetric_decrypt::decrypt(key, crypted_data);
}

void vds::security_walker::apply(
	const service_provider & parent_scope,
    const guid & channel_id,
    int message_id,
    const guid & read_cert_id,
    const guid & write_cert_id,
    const const_data_buffer & message_data,
    const const_data_buffer & signature){

	if (channel_id != this->user_id_) {
		return;
	}
	auto sp = parent_scope.create_scope(__FUNCTION__);
	auto log = sp.get<logger>();
	log->debug(ThisModule, sp, "Channel %s: Apply message %d, read cert %s, write_cert %s, message len %d, signature len %d",
		channel_id.str().c_str(),
		message_id,
		read_cert_id.str().c_str(),
		write_cert_id.str().c_str(),
		message_data.size(),
		signature.size());

  auto data = this->decrypt_message(
      parent_scope,
      channel_id,
      message_id,
      read_cert_id,
      write_cert_id,
      message_data,
      signature);

  binary_deserializer s(data);

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
        message.channel_id().str().c_str(),
				id.str().c_str());

      break;
    }

    case transactions::transaction_id::channel_add_writer_transaction: {
			transactions::channel_add_writer_transaction message(s);
			auto & cp = this->channels_[message.channel_id()];
			auto id = cert_control::get_id(message.write_cert());
			cp.name_ = message.name();
			cp.write_certificates_[id] = message.write_cert();
			cp.write_private_keys_[id] = message.write_private_key();
			cp.current_write_certificate_ = id;

			auto read_id = cert_control::get_id(message.read_cert());
			cp.read_certificates_[read_id] = message.read_cert();
			cp.current_read_certificate_ = read_id;

			log->debug(ThisModule, sp, "Got channel %s write certificate %s, read certificate %s",
				message.channel_id().str().c_str(),
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
    case transactions::transaction_id::channel_remove_reader_transaction: {
      break;
    }

    default:
      throw std::runtime_error("logic error");
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

