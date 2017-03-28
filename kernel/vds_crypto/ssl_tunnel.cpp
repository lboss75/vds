/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "ssl_tunnel.h"
#include "ssl_tunnel_p.h"
#include "crypto_exception.h"
#include "asymmetriccrypto.h"
#include "asymmetriccrypto_p.h"

static int verify_callback(int prev, X509_STORE_CTX * ctx)
{
  return 1;
}

////////////////////////////////////////////////////////////////
vds::ssl_tunnel::ssl_tunnel(
  const service_provider & scope,
  bool is_client,
  const certificate * cert,
  const asymmetric_private_key * key)
: impl_(new _ssl_tunnel(scope, this, is_client, cert, key))
{
}

vds::ssl_tunnel::~ssl_tunnel()
{
  delete this->impl_;
}

vds::certificate vds::ssl_tunnel::get_peer_certificate() const
{
  return this->impl_->get_peer_certificate();
}

void vds::ssl_tunnel::set_input_stream(issl_input_stream * stream)
{
  this->impl_->set_input_stream(stream);
}

void vds::ssl_tunnel::set_output_stream(issl_output_stream * stream)
{
  this->impl_->set_output_stream(stream);
}

void vds::ssl_tunnel::write_input(const void * data, size_t len)
{
  this->impl_->write_input(data, len);
}

void vds::ssl_tunnel::write_decoded_output(const void * data, size_t len)
{
  this->impl_->write_decoded_output(data, len);
}

void vds::ssl_tunnel::input_stream_processed()
{
  this->impl_->input_stream_processed();
}

void vds::ssl_tunnel::output_stream_processed()
{
  this->impl_->output_stream_processed();
}

vds::peer_certificate::peer_certificate(const vds::ssl_tunnel* owner)
: owner_(owner)
{
}

vds::certificate vds::peer_certificate::get_peer_certificate() const
{
  return this->owner_->get_peer_certificate();
}

////////////////////////////////////////////////////////////////
vds::_ssl_tunnel::_ssl_tunnel(
  const service_provider & scope,
  ssl_tunnel * owner,
  bool is_client,
  const certificate * cert,
  const asymmetric_private_key * key)
  : owner_(owner),
  is_client_(is_client),
  input_len_(0), decoded_input_len_(0),
  input_stream_(nullptr), output_stream_(nullptr),
  input_stream_done_(false),
  output_stream_done_(false),
  work_circle_queries_(0),
  enable_output_(true)
{
  this->ssl_ctx_ = SSL_CTX_new(is_client ? SSLv23_client_method() : SSLv23_server_method());
  if(nullptr == this->ssl_ctx_){
    auto error = ERR_get_error();
    throw new crypto_exception("SSL_CTX_new failed", error);
  }
  
  SSL_CTX_set_verify(this->ssl_ctx_, SSL_VERIFY_PEER, verify_callback);

  //set_certificate_and_key
  if (nullptr != cert) {
    int result = SSL_CTX_use_certificate(this->ssl_ctx_, cert->impl_->cert());
    if (0 >= result) {
      int ssl_error = SSL_get_error(this->ssl_, result);
      throw new crypto_exception("SSL_CTX_use_certificate", ssl_error);
    }

    result = SSL_CTX_use_PrivateKey(this->ssl_ctx_, key->impl_->key());
    if (0 >= result) {
      int ssl_error = SSL_get_error(this->ssl_, result);
      throw new crypto_exception("SSL_CTX_use_PrivateKey", ssl_error);
    }

    result = SSL_CTX_check_private_key(this->ssl_ctx_);
    if (0 >= result) {
      int ssl_error = SSL_get_error(this->ssl_, result);
      throw new crypto_exception("SSL_CTX_check_private_key", ssl_error);
    }
  }
  
  this->ssl_ = SSL_new(this->ssl_ctx_);
  if(nullptr == this->ssl_){
    auto error = ERR_get_error();
    throw new crypto_exception("SSL_new failed", error);
  }
  
  this->input_bio_ = BIO_new(BIO_s_mem());
  if(nullptr == this->input_bio_){
    auto error = ERR_get_error();
    throw new crypto_exception("BIO_new failed", error);
  }
  
  this->output_bio_ = BIO_new(BIO_s_mem());
  if(nullptr == this->output_bio_){
    auto error = ERR_get_error();
    throw new crypto_exception("BIO_new failed", error);
  }
  
  SSL_set_bio(this->ssl_, this->input_bio_, this->output_bio_);

  if (is_client) {
    SSL_set_connect_state(this->ssl_);
  }
  else {
    SSL_set_accept_state(this->ssl_);
  }
  
  scope.get<iscope_properties>().add_property(peer_certificate(this->owner_));
}

vds::_ssl_tunnel::~_ssl_tunnel()
{
}

vds::certificate vds::_ssl_tunnel::get_peer_certificate() const
{
  auto cert = SSL_get_peer_certificate(this->ssl_);

  if (nullptr == cert) {
    return certificate();
  }

  return certificate(new _certificate(cert));
}

void vds::_ssl_tunnel::set_input_stream(ssl_tunnel::issl_input_stream * stream)
{
  this->input_stream_ = stream;
}

void vds::_ssl_tunnel::set_output_stream(ssl_tunnel::issl_output_stream * stream)
{
  this->output_stream_ = stream;
}

void vds::_ssl_tunnel::write_input(const void * data, size_t len)
{
  if (0 != this->input_len_) {
    throw new std::logic_error("vds::ssl_tunnel::write_input");
  }

  this->input_data_ = data;
  this->input_len_ = len;

  this->start_work_circle();
}

void vds::_ssl_tunnel::write_decoded_output(const void * data, size_t len)
{
  if (0 != this->decoded_input_len_) {
    throw new std::logic_error("vds::ssl_tunnel::write_decoded_output");
  }

  this->decoded_input_data_ = data;
  this->decoded_input_len_ = len;

  this->start_work_circle();
}

void vds::_ssl_tunnel::start_work_circle()
{
  bool need_start;
  this->work_circle_mutex_.lock();
  need_start = (0 == this->work_circle_queries_++);
  this->work_circle_mutex_.unlock();
  
  if(need_start){
    this->work_circle();
  }
}

void vds::_ssl_tunnel::work_circle()
{
  for(;;) {
    if (nullptr != this->input_stream_ && 0 < this->input_len_) {
      int bytes = BIO_write(this->input_bio_, this->input_data_, (int)this->input_len_);
      if (bytes <= 0) {
        if (!BIO_should_retry(this->input_bio_)) {
          throw new std::runtime_error("BIO_write failed");
        }
      }
      else {
        this->input_data_ = reinterpret_cast<const uint8_t *>(this->input_data_) + bytes;
        this->input_len_ -= (size_t)bytes;

        if (this->input_len_ == 0) {
          this->input_stream_done_ = true;
        }
      }
    }

    if (0 < this->decoded_input_len_) {
      int bytes = SSL_write(this->ssl_, this->decoded_input_data_, (int)this->decoded_input_len_);
      if (0 > bytes) {
        int ssl_error = SSL_get_error(this->ssl_, bytes);
        switch (ssl_error) {
        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
          break;
        default:
          throw new crypto_exception("SSL_write", ssl_error);
        }
      }
      else {
        this->decoded_input_data_ = reinterpret_cast<const uint8_t *>(this->decoded_input_data_) + bytes;
        this->decoded_input_len_ -= (size_t)bytes;

        if (this->decoded_input_len_ == 0) {
          this->output_stream_done_ = true;
        }
      }
    }

    int bytes = SSL_read(this->ssl_, this->input_stream_->buffer_, ssl_tunnel::issl_input_stream::BUFFER_SIZE);
    if (bytes <= 0) {
      int ssl_error = SSL_get_error(this->ssl_, bytes);
      switch (ssl_error) {
      case SSL_ERROR_NONE:
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_WANT_CONNECT:
      case SSL_ERROR_WANT_ACCEPT:
      case SSL_ERROR_ZERO_RETURN:
        break;
      default:
        throw new crypto_exception("BIO_read", ssl_error);
      }
    }
    else {
      this->input_stream_->decoded_output_done(bytes);
    }

    if (this->enable_output_ && BIO_pending(this->output_bio_)) {
      int bytes = BIO_read(this->output_bio_, this->output_stream_->buffer_, ssl_tunnel::issl_output_stream::BUFFER_SIZE);
      if (bytes <= 0) {
        int ssl_error = SSL_get_error(this->ssl_, bytes);
        switch (ssl_error) {
        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
          break;
        default:
          throw new crypto_exception("BIO_read", ssl_error);
        }
      }
      else {
        this->enable_output_ = false;
        this->output_stream_->output_done(bytes);
      }
    }

    if (this->input_stream_done_ && this->input_stream_ != nullptr) {
      this->input_stream_done_ = false;
      this->input_stream_->input_done();
    }

    if (this->output_stream_done_ && this->output_stream_ != nullptr) {
      this->output_stream_done_ = false;
      this->output_stream_->decoded_input_done();
    }
    
    bool need_stop;
    this->work_circle_mutex_.lock();
    need_stop = (0 == --this->work_circle_queries_);
    this->work_circle_mutex_.unlock();
    if(need_stop) {
      break;
    }
  }
}

void vds::_ssl_tunnel::input_stream_processed()
{
  this->start_work_circle();
}

void vds::_ssl_tunnel::output_stream_processed()
{
  if (this->enable_output_) {
    throw new std::logic_error("vds::ssl_tunnel::output_stream_processed failed");
  }
  this->enable_output_ = true;
  this->start_work_circle();
}
