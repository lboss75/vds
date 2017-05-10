/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <assert.h>
#include "deferred_callback.h"

vds::_deferred_callback_base::_deferred_callback_base(deferred_context & context)
  : context_(context)
{
}

void vds::_deferred_callback_base::schedule()
{
  this->context_.schedule(this);
}

bool vds::_deferred_callback_base::direct_call_enabled()
{
  return false;
}

vds::deferred_context::deferred_context()
  : in_execute_(false)
{
}

vds::deferred_context::~deferred_context()
{
  assert(!this->in_execute_);
  assert(this->next_.empty());
}

void vds::deferred_context::operator()()
{
  this->this_mutex_.lock();
  this->continue_execute();
}

void vds::deferred_context::schedule(_deferred_callback_base * callback)
{
  this->this_mutex_.lock();
  this->next_.push(callback);
  if (this->in_execute_) {
    this->this_mutex_.unlock();
    return;
  }

  this->continue_execute();
}

void vds::deferred_context::continue_execute()
{
  auto keeper = this->shared_from_this();

  assert(!this->in_execute_);
  this->in_execute_ = true;

  while (!this->next_.empty()) {
    auto p = this->next_.top();
    this->next_.pop();
    this->this_mutex_.unlock();

    p->execute();

    this->this_mutex_.lock();
  }

  this->in_execute_ = false;
  this->this_mutex_.unlock();
}