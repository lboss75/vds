/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "inflate.h"
#include "private/inflate_p.h"

vds::inflate::inflate(stream<uint8_t> & target)
: stream<uint8_t>(new _inflate_handler(target))
{
}

//////////////////////////////////////////////////////
vds::inflate_async::inflate_async(stream_async<uint8_t> & target)
: stream_async<uint8_t>(new _inflate_async_handler(target))
{
}
