/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "ssl_tunnel.h"
#include "ssl_tunnel_p.h"
#include "asymmetriccrypto.h"
#include "asymmetriccrypto_p.h"

static int verify_callback(int prev, X509_STORE_CTX * ctx)
{
  return 1;
}

////////////////////////////////////////////////////////////////
vds::ssl_tunnel::ssl_tunnel(
  bool is_client,
  const certificate * cert,
  const asymmetric_private_key * key)
: impl_(new _ssl_tunnel(this, is_client, cert, key))
{
}

vds::ssl_tunnel::~ssl_tunnel()
{
  delete this->impl_;
}

////////////////////////////////////////////////////////////////
vds::_ssl_tunnel::_ssl_tunnel(
  ssl_tunnel * owner,
  bool is_client,
  const certificate * cert,
  const asymmetric_private_key * key)
: owner_(owner),
  is_client_(is_client),
  crypted_output_data_size_(0),
  crypted_input_data_size_(0),
  decrypted_output_data_size_(0),
  decrypted_input_data_size_(0),
  work_circle_continue_(false),
  work_circle_started_(false)
{
  this->ssl_ctx_ = SSL_CTX_new(is_client ? SSLv23_client_method() : SSLv23_server_method());
  if(nullptr == this->ssl_ctx_){
    auto error = ERR_get_error();
    throw crypto_exception("SSL_CTX_new failed", error);
  }
  
  SSL_CTX_set_verify(this->ssl_ctx_, SSL_VERIFY_PEER, verify_callback);

  //set_certificate_and_key
  if (nullptr != cert) {
    int result = SSL_CTX_use_certificate(this->ssl_ctx_, cert->impl_->cert());
    if (0 >= result) {
      int ssl_error = SSL_get_error(this->ssl_, result);
      throw crypto_exception("SSL_CTX_use_certificate", ssl_error);
    }

    result = SSL_CTX_use_PrivateKey(this->ssl_ctx_, key->impl_->key());
    if (0 >= result) {
      int ssl_error = SSL_get_error(this->ssl_, result);
      throw crypto_exception("SSL_CTX_use_PrivateKey", ssl_error);
    }

    result = SSL_CTX_check_private_key(this->ssl_ctx_);
    if (0 >= result) {
      int ssl_error = SSL_get_error(this->ssl_, result);
      throw crypto_exception("SSL_CTX_check_private_key", ssl_error);
    }
  }
  
  this->ssl_ = SSL_new(this->ssl_ctx_);
  if(nullptr == this->ssl_){
    auto error = ERR_get_error();
    throw crypto_exception("SSL_new failed", error);
  }
  
  this->input_bio_ = BIO_new(BIO_s_mem());
  if(nullptr == this->input_bio_){
    auto error = ERR_get_error();
    throw crypto_exception("BIO_new failed", error);
  }
  
  this->output_bio_ = BIO_new(BIO_s_mem());
  if(nullptr == this->output_bio_){
    auto error = ERR_get_error();
    throw crypto_exception("BIO_new failed", error);
  }
  
  SSL_set_bio(this->ssl_, this->input_bio_, this->output_bio_);

  if (is_client) {
    SSL_set_connect_state(this->ssl_);
  }
  else {
    SSL_set_accept_state(this->ssl_);
  }
  
  //scope.get<iscope_properties>().add_property(peer_certificate(this->owner_));
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

