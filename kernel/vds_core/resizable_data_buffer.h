#ifndef __VDS_CORE_RESIZABLE_DATA_BUFFER_H_
#define __VDS_CORE_RESIZABLE_DATA_BUFFER_H_

#include "const_data_buffer.h"

namespace vds {
  class resizable_data_buffer {
  public:
    resizable_data_buffer(){

    }

    resizable_data_buffer(size_t size){
      this->data_.resize(size);
    }

    resizable_data_buffer &operator += (const const_data_buffer & data){
      return this->add(data.data(), data.size());
    }

    resizable_data_buffer & add (const uint8_t * data, size_t size){
      auto offset = this->data_.size();
      this->data_.resize(offset + size);
      memcpy(this->data_.data() + offset, data, size);
      return *this;
    }

    uint8_t * data() {
      return this->data_.data();
    }
    const uint8_t * data() const {
      return this->data_.data();
    }

    size_t size() const {
      return this->data_.size();
    }

    void clear(){
      this->data_.clear();
    }

  private:
    std::vector<uint8_t> data_;
  };
}

#endif //__VDS_CORE_RESIZABLE_DATA_BUFFER_H_
