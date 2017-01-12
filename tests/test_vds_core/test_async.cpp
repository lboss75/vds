/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_async.h"


TEST(mt_tests, test_async) {
    vds::service_registrator registrator;

    vds::mt_service mt;
    vds::console_logger logger;

    registrator.add(logger);
    registrator.add(mt);

    test_async_object obj;
    vds::barrier barrier;

    {
        auto sp = registrator.build();

        vds::sequence(
        [&obj](const std::function<void(const std::string &)>& done, const vds::error_handler_t &, int value) {
            obj.sync_method(done, value);
        },
        [&obj, &sp](const std::function<void(void)> & done, const vds::error_handler_t & error_handler, const std::string & value) {
            obj.async_method(done, error_handler, sp, value);
        })([&obj]() {
            ASSERT_EQ(obj.state_, 2);
            obj.state_++;
        },
        [](std::exception * ex) {
            FAIL() << ex->what();
            delete ex;
        },
        10);

        sp.on_complete([&barrier] {
            barrier.set();
        });
    }


    barrier.wait();

    ASSERT_EQ(obj.state_, 3);

    registrator.shutdown();
}

test_async_object::test_async_object()
    : state_(0)
{
}

void test_async_object::sync_method(const std::function<void(const std::string &)> & done, int value)
{
    ASSERT_EQ(value, 10);
    ASSERT_EQ(this->state_, 0);

    this->state_++;
    done("test");
}

void test_async_object::async_method(const std::function<void(void)>& done, const vds::error_handler_t & error_handler, const vds::service_provider & sp,  const std::string & value)
{
    ASSERT_EQ(this->state_, 1);

    vds::async_result task;
    task.begin(sp, [this]() {
        ASSERT_EQ(this->state_, 1);

        this->state_++;
    }, done, error_handler);


}
