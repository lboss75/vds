#include "stdafx.h"
#include "stream.h"


vds::file_stream_output_async::file_stream_output_async(file&& f)
: f_(std::move(f)) {
}

vds::expected<std::shared_ptr<vds::file_stream_output_async>> vds::file_stream_output_async::create(const filename& fn, const file::file_mode mode) {
  file f;
  CHECK_EXPECTED(f.open(fn, mode));
  return std::make_shared<file_stream_output_async>(std::move(f));
}

vds::expected< std::shared_ptr<vds::file_stream_output_async>> vds::file_stream_output_async::create_tmp(const service_provider * sp) {
  file f;
  CHECK_EXPECTED(f.create_temp(sp));
  return std::make_shared<file_stream_output_async>(std::move(f));
}


vds::async_task<vds::expected<void>> vds::file_stream_output_async::write_async(const uint8_t* data, size_t len) {
  CHECK_EXPECTED_ASYNC(this->f_.write(data, len));
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::null_stream_output_async::write_async(const uint8_t* data, size_t len) {
  co_return vds::expected<void>();
}
