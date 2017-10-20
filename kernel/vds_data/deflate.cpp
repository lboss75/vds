/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/deflate_p.h"

vds::deflate::deflate(stream<uint8_t> & target)
  : impl_(new _deflate_handler(target, Z_DEFAULT_COMPRESSION))
{
}

vds::deflate::deflate(stream<uint8_t> & target, int compression_level)
  : impl_(new _deflate_handler(target, compression_level))
{
}

vds::deflate::~deflate()
{
  delete this->impl_;
}

void vds::deflate::write(const uint8_t * data, size_t len)
{
  this->impl_->write(data, len);
}

///////////////////////////////////////////////////////////////
vds::deflate_asynñ::deflate_asynñ(stream_asynñ<uint8_t> & target)
	: impl_(new _deflate_asynñ_handler(target, Z_DEFAULT_COMPRESSION))
{
}

vds::deflate_asynñ::deflate_asynñ(stream_asynñ<uint8_t> & target, int compression_level)
	: impl_(new _deflate_asynñ_handler(target, compression_level))
{
}

vds::deflate_asynñ::~deflate_asynñ()
{
	delete this->impl_;
}

vds::async_task<> vds::deflate_asynñ::write_async(const uint8_t * data, size_t len)
{
	return this->impl_->write_async(data, len);
}

