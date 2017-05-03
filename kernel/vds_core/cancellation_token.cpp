/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "cancellation_token.h"
#include "cancellation_token_p.h"

vds::cancellation_token::cancellation_token(
  const std::shared_ptr< _cancellation_token >& impl)
: impl_(impl)
{
}

vds::cancellation_token::~cancellation_token()
{
}

bool vds::cancellation_token::is_cancellation_requested() const
{
  return this->impl_->is_cancellation_requested();
}

void vds::cancellation_token::then_cancellation_requested(const std::function< void(void) >& callback)
{
  this->impl_->then_cancellation_requested(callback);
}

vds::cancellation_token_source::cancellation_token_source()
: impl_(new _cancellation_token())
{
}

vds::cancellation_token_source::~cancellation_token_source()
{
}


void vds::cancellation_token_source::cancel()
{
  this->impl_->cancel();
}

vds::cancellation_token vds::cancellation_token_source::token() const
{
  return cancellation_token(this->impl_);
}



