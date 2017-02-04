#ifndef __VDS_CORE_BARRIER_H_
#define __VDS_CORE_BARRIER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <condition_variable>
#include <mutex>

namespace vds {

    class barrier
    {
    public:
        barrier();
        ~barrier();

        void set();
        void wait();

    private:
        std::mutex mutex_;
        bool done_;
        std::condition_variable cond_;
    };
}

#endif // !__VDS_CORE_BARRIER_H_



