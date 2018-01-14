#include "stdafx.h"
#include "security_walker.h"
#include "channel_message.h"
#include "transactions/channel_message_transaction.h"
#include "certificate_private_key_dbo.h"

vds::security_walker::security_walker(
    const vds::guid &user_id,
    const vds::certificate &user_cert,
    const vds::asymmetric_private_key &user_private_key)
: user_id_(user_id),
  user_cert_(user_cert),
  user_private_key_(user_private_key){
}

void vds::security_walker::load(
    database_transaction &t) {

  dbo::channel_message t1;
  auto st = t.get_reader(
      t1.select(
              t1.channel_id,
              t1.message_id,
              t1.message,
              t1.read_cert_id,
              t1.write_cert_id,
              t1.signature)
          .where(t1.message_id ==
                 (int) transactions::channel_message_transaction::channel_message_id::channel_add_reader_transaction
                 || t1.message_id ==
                    (int) transactions::channel_message_transaction::channel_message_id::channel_remove_reader_transaction)
          .order_by(t1.id));

  while (st.execute()) {
    this->apply(
        t1.channel_id.get(st),
        t1.message_id.get(st),
        t1.read_cert_id.get(st),
        t1.write_cert_id.get(st),
        t1.message.get(st),
        t1.signature.get(st));
  }
}

void vds::security_walker::apply(
    const guid & channel_id,
    int message_id,
    const guid & read_cert_id,
    const guid & write_cert_id,
    const const_data_buffer & message_data,
    const const_data_buffer & signature) {

  auto &cp = this->channels_[channel_id];

  auto p = cp.certificates_.find(write_cert_id);
  if (cp.certificates_.end() == p) {
    if (channel_id == this->user_id_) {
      throw std::runtime_error(
          string_format(
              "Unknown write certificate %s",
              write_cert_id.str().c_str()));
    } else {
      return;
    }
  }
  if (!asymmetric_sign_verify::verify(
      hash::sha256(),
      p->second.public_key(),
      signature,
      (binary_serializer()
          << (uint8_t)message_id
          << this->user_id_
          << read_cert_id
          << write_cert_id
          << message_data).data())) {
    throw std::runtime_error("Write signature error");
  }

  auto p1 = cp.read_private_keys_.find(read_cert_id);
  if (cp.read_private_keys_.end() == p1) {
    if (channel_id == this->user_id_) {
      throw std::runtime_error(
          string_format(
              "Unknown read certificate %s",
              read_cert_id.str().c_str()));
    } else {
      return;
    }
  }

  binary_deserializer message_stream(message_data);
  const_data_buffer key_data;
  const_data_buffer crypted_data;

  message_stream >> key_data >> crypted_data;

  auto key_info = p1->second.decrypt(key_data);
  auto key = symmetric_key::deserialize(
      symmetric_crypto::aes_256_cbc(),
      binary_deserializer(key_info));

  auto data = symmetric_decrypt::decrypt(key, crypted_data);
  binary_deserializer s(data);

  switch ((transactions::channel_message_transaction::channel_message_id) message_id) {
    case transactions::channel_message_transaction::channel_message_id::channel_add_reader_transaction: {
      const_data_buffer read_cert_der;
      const_data_buffer read_private_key_der;
      s >> read_cert_der >> read_private_key_der;

      auto read_cert = certificate::parse_der(read_cert_der);
      auto read_private_key = asymmetric_private_key::parse_der(read_private_key_der, std::string());

      auto id = cert_control::get_id(read_cert);
      cp.certificates_[id] = read_cert;
      cp.read_private_keys_[id] = read_private_key;
      break;
    }

    case transactions::channel_message_transaction::channel_message_id::channel_remove_reader_transaction: {
      break;
    }
  }
}

