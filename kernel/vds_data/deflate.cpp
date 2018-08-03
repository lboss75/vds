/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/deflate_p.h"
#include "const_data_buffer.h"

vds::deflate::deflate(stream<uint8_t> & target)
  : stream<uint8_t>(new _deflate_handler(target, Z_DEFAULT_COMPRESSION))
{
}

vds::deflate::deflate(stream<uint8_t> & target, int compression_level)
  : stream<uint8_t>(new _deflate_handler(target, compression_level))
{
}


vds::const_data_buffer vds::deflate::compress(
  const uint8_t * data,
  size_t len)
{
  collect_data<uint8_t> result;
  deflate df(result);
  
  df.write(data, len);
  df.write(nullptr, 0);
  
  return result.get_data();  
}

vds::const_data_buffer vds::deflate::compress(
  const const_data_buffer & data)
{
  return compress(data.data(), data.size());
}
///////////////////////////////////////////////////////////////
vds::deflate_async::deflate_async(stream_async<uint8_t> & target)
	: stream_async<uint8_t>(new _deflate_async_handler(target, Z_DEFAULT_COMPRESSION))
{
}

vds::deflate_async::deflate_async(stream_async<uint8_t> & target, int compression_level)
	: stream_async<uint8_t>(new _deflate_async_handler(target, compression_level))
{
}
