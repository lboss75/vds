#ifndef __TEST_VDS_CORE_TEST_ASYNC_H_
#define __TEST_VDS_CORE_TEST_ASYNC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

class test_async_object
{
public:
    test_async_object();

    void sync_method(
        const std::function<void(const std::string & )>& done,
        int value);

    void async_method(
        const std::function<void(void)> & done,
        const vds::error_handler_t & error_handler,
        const vds::service_provider & sp,
        const std::string & value);

    int state_;
};


#endif // !__TEST_VDS_CORE_TEST_ASYNC_H_

