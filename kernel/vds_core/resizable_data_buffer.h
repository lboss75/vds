#ifndef __VDS_CORE_RESIZABLE_DATA_BUFFER_H_
#define __VDS_CORE_RESIZABLE_DATA_BUFFER_H_

#include "const_data_buffer.h"

namespace vds {
  class resizable_data_buffer {
  public:
    resizable_data_buffer(){

    }

    resizable_data_buffer &operator += (const const_data_buffer & data){
      return this->add(data.data(), data.size());
    }

    resizable_data_buffer & add (const uint8_t * data, size_t size){
      this->data_.push_back(const_data_buffer(data, size));
      return *this;
    }

    resizable_data_buffer & add(const uint8_t value) {
      this->data_.push_back(const_data_buffer(&value, sizeof(value)));
      return *this;
    }

    const_data_buffer get_data() const {
      return const_data_buffer(this->data_);
    }

    void clear(){
      this->data_.clear();
    }

  private:
    std::list<const_data_buffer> data_;
  };
}

#endif //__VDS_CORE_RESIZABLE_DATA_BUFFER_H_
