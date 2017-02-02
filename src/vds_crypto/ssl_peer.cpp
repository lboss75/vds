/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "ssl_peer.h"
#include "crypto_exception.h"
#include "asymmetriccrypto.h"

vds::ssl_peer::ssl_peer(bool is_client)
  : is_client_(is_client), is_handshaking_(true)
{
  this->ssl_ctx_ = SSL_CTX_new(SSLv23_method());
  SSL_CTX_set_verify(this->ssl_ctx_, SSL_VERIFY_NONE, nullptr);

  this->ssl_ = SSL_new(this->ssl_ctx_);
  this->input_bio_ = BIO_new(BIO_s_mem());
  this->output_bio_ = BIO_new(BIO_s_mem());
  SSL_set_bio(this->ssl_, this->input_bio_, this->output_bio_);

  if (is_client) {
    SSL_set_connect_state(this->ssl_);
  }
  else {
    SSL_set_accept_state(this->ssl_);
  }

}

size_t vds::ssl_peer::write_input(const void * data, size_t len)
{
  int bytes = BIO_write(this->input_bio_, data, len);
  if (bytes <= 0) {
    if (!BIO_should_retry(this->input_bio_))
    {
      throw new std::runtime_error("BIO_write failed");
    }

    return len;
  }
  else {
    return (size_t)bytes;
  }
}

size_t vds::ssl_peer::read_decoded(uint8_t * data, size_t len)
{
  int bytes = SSL_read(this->ssl_, data, len);
  if (this->is_handshaking_ && SSL_is_init_finished(this->ssl_)) {
    this->is_handshaking_ = false;
  }

  return (size_t)bytes;
}

bool vds::ssl_peer::write_decoded(const void * data, size_t len)
{
  int bytes = SSL_write(this->ssl_, data, len);
  int ssl_error = SSL_get_error(this->ssl_, bytes);
  if (len == bytes) {
    return true;
  }
  else {
    switch (ssl_error) {
    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_CONNECT:
    case SSL_ERROR_WANT_ACCEPT:
      return false;
    }

    throw new crypto_exception("SSL_write", ssl_error);
  }
}

size_t vds::ssl_peer::read_output(uint8_t * data, size_t len)
{
  int bytes = BIO_read(this->output_bio_, data, len);
  if (bytes > 0) {
    return (size_t)bytes;
  }
  else {
    int ssl_error = SSL_get_error(this->ssl_, bytes);
    switch (ssl_error) {
    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_CONNECT:
    case SSL_ERROR_WANT_ACCEPT:
      return 0;
    }

    throw new crypto_exception("BIO_read", ssl_error);
  }
}

void vds::ssl_peer::set_certificate(const certificate & cert)
{
  SSL_CTX_use_certificate(this->ssl_ctx_, cert.cert());
}

