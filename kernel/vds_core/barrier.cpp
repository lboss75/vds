/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "barrier.h"


vds::barrier::barrier(size_t init_value)
: count_(init_value)
{
}


vds::barrier::~barrier()
{
}

void vds::barrier::set()
{
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->count_ = 0;
    this->cond_.notify_all();
}

void vds::barrier::wait()
{
    std::unique_lock<std::mutex> lock(this->mutex_);

    while (0 != this->count_) {
        this->cond_.wait(lock,
        [this] { return 0 == this->count_; });
    }
}

void vds::barrier::reset(size_t init_value)
{
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->count_ = init_value;
}

vds::barrier& vds::barrier::operator++()
{
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->count_++;
    return *this;
}

vds::barrier& vds::barrier::operator--()
{
    std::unique_lock<std::mutex> lock(this->mutex_);
    if(0 == --(this->count_)){
      this->cond_.notify_all();
    }
    
    return *this;

}
