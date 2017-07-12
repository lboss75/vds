/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "debug.h"

void vds::debug_task_type::dump(std::ostream & s) const
{
  s << "{";
  std::lock_guard<std::mutex> lock(this->items_mutex_);
  for (auto p : this->items_) {
    s << p.c_str();
    s << ";";
  }
  s << "}";
}
