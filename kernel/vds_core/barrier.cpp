/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "barrier.h"


vds::barrier::barrier()
: done_(false)
{
}


vds::barrier::~barrier()
{
}

void vds::barrier::set()
{
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->done_ = true;
    this->cond_.notify_all();
}

void vds::barrier::wait()
{
    std::unique_lock<std::mutex> lock(this->mutex_);

    while (!this->done_) {
        this->cond_.wait(lock,
        [this] { return this->done_; });
    }
}

void vds::barrier::reset()
{
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->done_ = false;
}
