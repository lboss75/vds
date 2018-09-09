#ifndef __VDS_CORE_RESIZABLE_DATA_BUFFER_H_
#define __VDS_CORE_RESIZABLE_DATA_BUFFER_H_

#include "const_data_buffer.h"
#include <cstdlib>

namespace vds {
  class resizable_data_buffer {
  public:
    resizable_data_buffer()
    : data_(nullptr), size_(0), allocated_size_(0) {

    }

    ~resizable_data_buffer() {
      if(this->data_ != nullptr) {
        std::free(this->data_);
      }
    }

    resizable_data_buffer &operator += (const const_data_buffer & data){
      return this->add(data.data(), data.size());
    }

    resizable_data_buffer & add (const void * data, size_t size){
      const auto new_size = this->size_ + size;
      this->resize_data(new_size);
      memcpy(this->data_ + this->size_, data, size);
      this->size_ = new_size;
      return *this;
    }

    resizable_data_buffer & add(const uint8_t value) {
      this->resize_data(this->size_ + 1);
      this->data_[this->size_++] = value;
      return *this;
    }

    const_data_buffer move_data() {
      return const_data_buffer(std::move(*this));
    }

    const uint8_t * data() const {
      return this->data_;
    }

    size_t size() const {
      return this->size_;
    }

    void clear(){
      this->size_ = 0;
    }

  private:
    friend class const_data_buffer;

    uint8_t * data_;
    size_t size_;
    size_t allocated_size_;

    void resize_data(size_t size) {
      if (this->allocated_size_ < size) {
        this->allocated_size_ = 1024 * ((size + 1023) / 1024);
        this->data_ = static_cast<uint8_t *>(std::realloc(this->data_, this->allocated_size_));
        if (nullptr == this->data_) {
          throw std::runtime_error("Out of memmory");
        }
      }
    }
  };
}

#endif //__VDS_CORE_RESIZABLE_DATA_BUFFER_H_
