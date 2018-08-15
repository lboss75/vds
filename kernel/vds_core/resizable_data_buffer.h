#ifndef __VDS_CORE_RESIZABLE_DATA_BUFFER_H_
#define __VDS_CORE_RESIZABLE_DATA_BUFFER_H_

#include "const_data_buffer.h"

namespace vds {
  class resizable_data_buffer {
  public:
    resizable_data_buffer()
    : data_(nullptr), size_(0), allocated_(0) {

    }

    ~resizable_data_buffer() {
      if(this->data_ != nullptr) {
        free(this->data_);
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

    const_data_buffer get_data() const {
      return const_data_buffer(this->data_, this->size_);
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
    uint8_t * data_;
    size_t size_;
    size_t allocated_;

    void resize_data(size_t size) {
      if (this->allocated_ < size) {
        this->allocated_ = 1024 * ((size + 1023) / 1024);
        this->data_ = static_cast<uint8_t *>(realloc(this->data_, this->allocated_));
        if (nullptr == this->data_) {
          throw std::runtime_error("Out of memmory");
        }
      }
    }
  };
}

#endif //__VDS_CORE_RESIZABLE_DATA_BUFFER_H_
