/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_events.h"

TEST(core_events, test_events) {
  source_class source;
  target_class * target = new target_class();

  source.source += target->target;

  source.source();
}

target_class::target_class()
  : target([this]() {delete this; })
{
}
