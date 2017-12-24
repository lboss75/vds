//
// Created by vadim on 12.11.17.
//

#include <cert_control.h>
#include "p2p_crypto_tunnel.h"
#include "p2p_network.h"
#include "private/p2p_crypto_tunnel_p.h"
#include "private/p2p_network_p.h"

void vds::p2p_crypto_tunnel::start(
    const service_provider & sp) {

  static_cast<_p2p_crypto_tunnel *>(this->impl_.get())->start(sp);

}

vds::p2p_crypto_tunnel::p2p_crypto_tunnel(const std::shared_ptr<vds::udp_transport::_session> &impl)
: session(impl) {

}

vds::p2p_crypto_tunnel::~p2p_crypto_tunnel() {
}

////////////////////////////////////////////////////////
void vds::_p2p_crypto_tunnel::close(
    const service_provider &sp,
    const std::shared_ptr<std::exception> &ex) {
  this->session_->close(sp, ex);
}

void vds::_p2p_crypto_tunnel::start(
    const service_provider &sp) {

  this->read_input_messages(sp);
}

void vds::_p2p_crypto_tunnel::read_input_messages(const service_provider &sp) {
  session_.read_async(sp).execute(
      [pthis = shared_from_this(), sp](
          const std::shared_ptr<std::exception> & ex,
          const const_data_buffer & message){
        if(!ex) {
          auto this_ = static_cast<_p2p_crypto_tunnel *>(pthis.get());
          this_->process_input_command(sp, message);
          this_->read_input_messages(sp);
        }
      });
}

void vds::_p2p_crypto_tunnel::process_input_command(
    const service_provider &sp,
    const const_data_buffer &message) {
  binary_deserializer s(message);

  uint8_t command;
  s >> command;
  this->process_input_command(sp, (command_id)command, s);
}

void vds::_p2p_crypto_tunnel::send(
    const service_provider &sp,
    const const_data_buffer &message) {

  binary_serializer s;
  if(!this->output_key_) {
    if (!this->input_key_) {
      throw std::runtime_error("Invalid state");
    }
    s << (uint8_t) command_id::CryptedByInput;
    s << symmetric_encrypt::encrypt(this->input_key_, message);
  }
  else {
    s << (uint8_t) command_id::Crypted;
    s << symmetric_encrypt::encrypt(this->output_key_, message);
  }

  this->session_->send(
      sp,
      const_data_buffer(s.data().data(), s.size()));
}

vds::async_task<const vds::const_data_buffer &> vds::_p2p_crypto_tunnel::read_async(
    const service_provider &sp) {
  return [pthis = this->shared_from_this()](
      const async_result<const const_data_buffer &> & result){
    static_cast<_p2p_crypto_tunnel *>(pthis.get())->data_read_result_ = result;
  };
}

void vds::_p2p_crypto_tunnel::process_input_command(
    const service_provider &sp,
    const command_id command,
    binary_deserializer &s) {
  switch(command) {
    case command_id::CertCain: {
      sp.get<logger>()->trace("P2PUDPAPI", sp, "Got CertCain");
      uint16_t size;
      s >> size;

      std::list<certificate> certificate_chain;
      for (decltype(size) i = 0; i < size; ++i) {
        const_data_buffer buffer;
        s >> buffer;
        certificate_chain.push_back(certificate::parse_der(buffer));
      }

      //TODO: validate chain

      if (!this->output_key_) {
        this->partner_id_ = cert_control::get_id(*certificate_chain.rbegin());
        this->output_key_ = symmetric_key::generate(symmetric_crypto::aes_256_cbc());
      } else {
        sp.get<logger>()->warning("P2PUDPAPI", sp, "Duplicate CertCain");
        return;
      }

      binary_serializer key_stream;
      this->output_key_.serialize(key_stream);

      auto crypted_key = certificate_chain.rbegin()->public_key().encrypt(
          key_stream.data().data(),
          key_stream.size());

      binary_serializer out_stream;
      out_stream << (uint8_t)command_id::SendKey << safe_cast<uint16_t>(crypted_key.size());
      out_stream.push_data(crypted_key.data(), crypted_key.size(), false);

      this->session_.send(sp, const_data_buffer(out_stream.data().data(), out_stream.size()));
      return;
    }
    case command_id::Crypted:{
      if(!this->input_key_){
        throw std::runtime_error("Invalid state");
      }

      const_data_buffer message;
      s >> message;

      this->process_input_command(
          sp,
          symmetric_decrypt::decrypt(
              this->input_key_,
              message.data(), message.size()));
      break;
    }
    case command_id::SendKey: {
      sp.get<logger>()->trace("P2PUDPAPI", sp, "Got SendKey");

      uint16_t size;
      s >> size;

      resizable_data_buffer crypted_key(size);
      size_t lsize = size;
      s.pop_data(crypted_key.data(), lsize, false);

      auto key_data = this->private_key_.decrypt(crypted_key.data(), size);
      binary_deserializer key_stream(key_data);

      std::unique_lock<std::mutex> lock(this->key_mutex_);
      this->input_key_ = symmetric_key::deserialize(
          symmetric_crypto::aes_256_cbc(),
          key_stream);

      if(this->output_key_){
        lock.unlock();

        (*sp.get<p2p_network>())->add_route(
            sp,
            this->partner_id_,
            this->shared_from_this());
      }
      break;
    }
    case command_id::CryptedByInput:{
      if(!this->output_key_){
        throw std::runtime_error("Invalid state");
      }

      const_data_buffer message;
      s >> message;

      this->process_input_command(
          sp,
          symmetric_decrypt::decrypt(
              this->output_key_,
              message.data(), message.size()));
      break;
    }
    case command_id::Data: {
      if(!this->input_key_){
        throw std::runtime_error("Invalid state");
      }

      this->data_read_result_.done(
          symmetric_decrypt::decrypt(this->input_key_, s.data(), s.size()));
      break;
    }
    default:
      throw std::runtime_error("Invalid command");
  }
}
