/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "peer_channel.h"
#include "peer_channel_p.h"

vds::peer_channel::peer_channel(_peer_channel * impl)
  : impl_(impl)
{
}

vds::peer_channel::~peer_channel()
{
  delete this->impl_;
}
