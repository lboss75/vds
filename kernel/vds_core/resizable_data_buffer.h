#ifndef __VDS_CORE_RESIZABLE_DATA_BUFFER_H_
#define __VDS_CORE_RESIZABLE_DATA_BUFFER_H_

#include "const_data_buffer.h"
#include <cstdlib>
#include <assert.h>
#include "expected.h"

namespace vds {
  class resizable_data_buffer {
  public:
    resizable_data_buffer()
    : data_(nullptr), size_(0), allocated_size_(0) {

    }

    resizable_data_buffer(resizable_data_buffer && other) noexcept
      : data_(other.data_), size_(other.size_), allocated_size_(other.allocated_size_) {
      other.data_ = nullptr;
      other.size_ = 0;
      other.allocated_size_ = 0;
    }

    resizable_data_buffer(const resizable_data_buffer & other) = delete;
    resizable_data_buffer & operator = (const resizable_data_buffer & other) = delete;
    resizable_data_buffer & operator = (resizable_data_buffer && other) = delete;

    ~resizable_data_buffer() {
      if(this->data_ != nullptr) {
        std::free(this->data_);
      }
    }

    expected<void> add(const const_data_buffer & data) {
      return this->add(data.data(), data.size());
    }

    expected<void> add (const void * data, size_t size){
      const auto new_size = this->size_ + size;
      CHECK_EXPECTED(this->resize_data(new_size));
      memcpy(this->data_ + this->size_, data, size);
      this->size_ = new_size;
      return expected<void>();
    }

    expected<void> add(const uint8_t value) {
      CHECK_EXPECTED(this->resize_data(this->size_ + 1));
      this->data_[this->size_++] = value;
      return expected<void>();
    }

    expected<void> add_uint32(const uint32_t value) {
      CHECK_EXPECTED(this->resize_data(this->size_ + 4));
      this->data_[this->size_++] = 0xFF & (value >> 24);
      this->data_[this->size_++] = 0xFF & (value >> 16);
      this->data_[this->size_++] = 0xFF & (value >> 8);
      this->data_[this->size_++] = 0xFF & value;
      return expected<void>();
    }


    const_data_buffer move_data() {
      return const_data_buffer(std::move(*this));
    }

    const uint8_t * data() const {
      return this->data_;
    }

    uint8_t & operator[](size_t index) {
      assert(0 <= index && index < this->size_);
      return this->data_[index];
    }

    uint8_t * data() {
      return this->data_;
    }

    size_t size() const {
      return this->size_;
    }

    void clear(){
      this->size_ = 0;
    }

    void apply_size(size_t size) {
      vds_assert(this->size_ + size <= this->allocated_size_);
      this->size_ += size;
    }

    [[nodiscard]] 
    expected<void> resize_data(size_t size) {
      if (this->allocated_size_ < size) {
        const auto new_size = DELTA_SIZE * ((size + DELTA_SIZE - 1) / DELTA_SIZE);
        auto p = std::realloc(this->data_, new_size);
        if (nullptr == p) {
            return make_unexpected<std::bad_alloc>();
        }
        this->allocated_size_ = new_size;
        this->data_ = static_cast<uint8_t *>(p);
      }
      return expected<void>();
    }

    void remove(size_t offset, size_t len) {
      vds_assert(offset + len <= this->size_);
      if (0 < len) {
        if (offset + len < this->size_) {
          memmove(this->data_ + offset, this->data_ + offset + len, this->size_ - offset - len);
        }
        this->size_ -= len;
      }
    }

  private:
    friend class const_data_buffer;
    static constexpr size_t DELTA_SIZE = 8 * 1024;

    uint8_t * data_;
    size_t size_;
    size_t allocated_size_;

  };
}

#endif //__VDS_CORE_RESIZABLE_DATA_BUFFER_H_
