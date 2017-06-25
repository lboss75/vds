/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "object_transfer_protocol.h"
#include "object_transfer_protocol_p.h"

vds::object_transfer_protocol::object_transfer_protocol()
: impl_(new _object_transfer_protocol())
{

}

vds::object_transfer_protocol::~object_transfer_protocol()
{
  delete this->impl_;
}


//////////////////////////////////////////////////////
vds::_object_transfer_protocol::_object_transfer_protocol()
{

}

vds::_object_transfer_protocol::~_object_transfer_protocol()
{
}
