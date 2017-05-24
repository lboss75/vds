#include "stdafx.h"
#include "test_async_stream.h"
#include "mt_service.h"
#include "random_reader.h"
#include "compare_data.h"
#include "async_task.h"
#include "async_stream.h"
#include "random_buffer.h"

TEST(mt_tests, test_async_stream) {
  vds::service_registrator registrator;

  vds::console_logger logger(vds::ll_trace);
  vds::mt_service mt_service;

  registrator.add(logger);
  registrator.add(mt_service);

  auto sp = registrator.build("test_async_stream");
  registrator.start(sp);

  random_buffer data;
  auto stream = std::make_shared<vds::async_stream<uint8_t>>();

  vds::barrier b;
  std::exception_ptr err;

  vds::async_series({
    vds::create_async_task(
      [&data, stream](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & error,
        const vds::service_provider & sp) {
        vds::mt_service::async(sp, [&data, stream, done, error, sp] {
          vds::dataflow(
            random_reader<uint8_t>(data.data(), data.size()),
            vds::stream_write<uint8_t>(stream)
          )(done, error, sp);
        });
      }
    ),
    vds::create_async_task(
      [&data, stream](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & error,
        const vds::service_provider & sp) {
        vds::mt_service::async(sp, [&data, stream, done, error, sp] {
          vds::dataflow(
            vds::stream_read<uint8_t>(stream),
            compare_data<uint8_t>(data.data(), data.size())
          )(done, error, sp);
        });
      }
    )
  })
    .wait(
      [&b](const vds::service_provider & sp) {
        b.set();
      },
      [&b, &err](const vds::service_provider & sp, std::exception_ptr ex) {
        err = ex;
        b.set();
      },
        sp
        );

      b.wait();
      registrator.shutdown(sp);
      if (err) {
        GTEST_FAIL() << vds::exception_what(err);
      }
}
