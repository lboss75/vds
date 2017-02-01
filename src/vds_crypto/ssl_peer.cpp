/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "ssl_peer.h"
#include "crypto_exception.h"

vds::ssl_peer::ssl_peer(bool is_client)
  : is_client_(is_client), is_handshaking_(true)
{
  this->ssl_ctx_ = SSL_CTX_new(SSLv23_method());
  SSL_CTX_set_verify(this->ssl_ctx_, SSL_VERIFY_NONE, nullptr);

  this->ssl_ = SSL_new(this->ssl_ctx_);
  this->read_bio_ = BIO_new(BIO_s_mem());
  this->write_bio_ = BIO_new(BIO_s_mem());
  SSL_set_bio(this->ssl_, this->read_bio_, this->write_bio_);

  if (is_client) {
    SSL_set_connect_state(this->ssl_);
  }
  else {
    SSL_set_accept_state(this->ssl_);
  }

}

bool vds::ssl_peer::write_input(const void * data, size_t len)
{
  int bytes = BIO_write(this->read_bio_, data, len);
  if (bytes == len) {
    return true;
  }
  else
  if (BIO_should_retry(this->read_bio_))
  {
    return false;
  }

  throw new std::runtime_error("BIO_write failed");
}

size_t vds::ssl_peer::read_input(uint8_t * data, size_t len)
{
  int bytes = SSL_read(this->ssl_, data, len);
  if (this->is_handshaking_ && SSL_is_init_finished(this->ssl_)) {
    this->is_handshaking_ = false;
  }

  return (size_t)bytes;
}

bool vds::ssl_peer::write_output(const void * data, size_t len)
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
  int bytes = BIO_read(this->write_bio_, data, len);
  int ssl_error = SSL_get_error(this->ssl_, bytes);
  if (bytes > 0) {
    return (size_t)bytes;
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

    throw new crypto_exception("BIO_read", ssl_error);
  }
}

