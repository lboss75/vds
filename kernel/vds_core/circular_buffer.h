#ifndef __VDS_CORE_CIRCULAR_BUFFER_H_
#define __VDS_CORE_CIRCULAR_BUFFER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  template <typename owner_class, uint32_t buffer_size, uint32_t min_buffer_size>
  class circular_buffer
  {
  public:
    circular_buffer(
      owner_class * owner)
      : owner_(owner), second_(0), front_(0), back_(0),
      data_queried_(false), data_in_process_(false)
    {
    }

    void start()
    {
      this->query_data();
    }

    void dequeue() {
      std::unique_lock<std::mutex> lock(this->buffer_mutex_);
      this->data_in_process_ = false;
      if (this->front_ < this->back_) {
        auto len = *(uint32_t *)(this->buffer_ + this->front_);
        this->front_ += len + 4;
      }

      if (this->front_ == this->back_ && !this->data_queried_ && !this->data_in_process_) {
        this->front_ = 0;
        this->back_ = this->second_;
        this->second_ = 0;
      }

      if (!this->data_queried_) {
        this->query_data();
      }

      this->push_data(lock);
    }

    void queue(uint32_t len) {
      std::unique_lock<std::mutex> lock(this->buffer_mutex_);

      this->data_queried_ = false;
      if (this->back_ + min_buffer_size + 4 < buffer_size) {

        *(uint32_t *)(this->buffer_ + this->back_) = len;
        this->back_ += len + 4;

        this->query_data();
      }
      else {
        if (this->front_ == this->back_ && !this->data_queried_ && !this->data_in_process_) {
          this->front_ = 0;
          this->back_ = this->second_;
          this->second_ = 0;
        }

        if (this->second_ + min_buffer_size + 4 < this->front_) {
          *(uint32_t *)(this->buffer_ + this->second_) = len;
          this->second_ += len + 4;

          this->query_data();
        }
        else {
          throw std::runtime_error("Invalid logic");
        }
      }

      if (!this->data_in_process_) {
        this->push_data(lock);
      }
    }

  private:
    std::mutex buffer_mutex_;
    owner_class * owner_;

    uint8_t buffer_[buffer_size];
    uint32_t second_;
    uint32_t front_;
    uint32_t back_;

    bool data_queried_;
    bool data_in_process_;

    //            0    second   front    back   buffer_size
    // to read    [...2...]       [...1...]
    // to write            [..2..]         [...1...]

    bool push_data(std::unique_lock<std::mutex> & lock)
    {
      assert(this->data_in_process_ == false);

      if (this->front_ < this->back_) {
        auto len = *(uint32_t *)(this->buffer_ + this->front_);
        this->data_in_process_ = true;

        auto p = this->buffer_ + this->front_ + 4;
        lock.unlock();

        this->owner_->push_data(p, len);
        return true;
      }

      return false;
    }

    bool query_data()
    {
      assert(this->data_queried_ == false);

      if (this->back_ + min_buffer_size + 4 < buffer_size) {
        this->data_queried_ = true;
        owner_->data_require(this->buffer_ + this->back_ + 4, buffer_size - this->back_ - 4);
        return true;
      }

      if (this->second_ + min_buffer_size + 4 < this->front_) {
        this->data_queried_ = true;
        owner_->data_require(this->buffer_ + this->second_ + 4, this->front_ - this->second_ - 4);
        return true;
      }

      return false;
    }
  };
}

#endif // __VDS_CORE_CIRCULAR_BUFFER_H_
