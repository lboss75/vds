#include "stdafx.h"
#include "stream.h"

vds::file_stream_output_async::file_stream_output_async(const filename& fn, const file::file_mode mode)
: own_file_(fn, mode), f_(&own_file_) {
}

vds::file_stream_output_async::file_stream_output_async(file* f)
: f_(f) {
}

vds::file_stream_output_async::~file_stream_output_async() {
}

vds::async_task<void> vds::file_stream_output_async::write_async(const uint8_t* data, size_t len) {
  this->f_->write(data, len);
  co_return;
}

vds::async_task<void> vds::null_stream_output_async::write_async(const uint8_t* data, size_t len) {
  co_return;
}
