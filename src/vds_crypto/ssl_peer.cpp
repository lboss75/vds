/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "ssl_peer.h"

vds::ssl_peer::ssl_peer(bool is_client)
{
  this->ssl_ctx_ = SSL_CTX_new(SSLv23_method());
  SSL_CTX_set_verify(this->ssl_ctx_, SSL_VERIFY_NONE, nullptr);

  this->ssl_ = SSL_new(this->ssl_ctx_);
  this->read_bio_ = BIO_new(BIO_s_mem());
  this->write_bio_ = BIO_new(BIO_s_mem());
  SSL_set_bio(this->ssl_, this->read_bio_, this->write_bio_);
}
