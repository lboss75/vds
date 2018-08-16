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

vds::const_data_buffer vds::inflate::decompress(const void * data, size_t size)
{
	collect_data<uint8_t> result;
	inflate inf(result);

	inf.write((const uint8_t *)data, size);
	inf.write(nullptr, 0);

	return result.move_data();
}

//////////////////////////////////////////////////////
vds::inflate_async::inflate_async(stream_async<uint8_t> & target)
: stream_async<uint8_t>(new _inflate_async_handler(target))
{
}
