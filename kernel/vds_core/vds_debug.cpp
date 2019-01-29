/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "vds_debug.h"

static thread_local bool thread_protected = false;

vds::thread_protect::thread_protect() {
  vds_assert(!thread_protected);
  thread_protected = true;
}

vds::thread_protect::~thread_protect() {
  vds_assert(thread_protected);
  thread_protected = false;
}

void vds::thread_protect::check() {
  vds_assert(!thread_protected);
}

vds::thread_unprotect::thread_unprotect() {
  this->original_ = thread_protected;
  thread_protected = false;
}

vds::thread_unprotect::~thread_unprotect() {
  thread_protected = this->original_;
}
