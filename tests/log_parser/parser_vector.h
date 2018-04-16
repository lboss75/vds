//
// Created by vadim on 16.04.18.
//

#ifndef VDS_PARSER_VECTOR_H
#define VDS_PARSER_VECTOR_H

#include <cstdlib>
#include <cstring>

template <typename item_type>
class parser_vector {
public:
  parser_vector()
  : data_(nullptr), size_(0), count_(0){
  }

  parser_vector(parser_vector && origin)
  : data_(origin.data_), size_(origin.size_), count_(origin.count_){
    origin.data_ = nullptr;
  }

  ~parser_vector(){
    if(nullptr != data_){
      free(data_);
    }
  }

  void clear(){
    if(nullptr != data_){
      free(data_);
      data_ = nullptr;
    }
  }

  bool add(item_type && val){
    if(this->size_ == this->count_){
      auto new_data = (item_type *)malloc(sizeof(item_type) * (this->size_ + 5));
      if(nullptr == new_data){
        return false;//Out of memory
      }

      if(nullptr != data_) {
        memcpy(new_data, data_, sizeof(item_type) * this->size_);
        free(this->data_);
      }

      this->data_ = new_data;
      this->size_ += 5;
    }

    this->data_[this->count_++] = static_cast<item_type &&>(val);
    return true;
  }

  const item_type & operator[](size_t index) const {
    return this->data_[index];
  }

  item_type & operator[](size_t index) {
    return this->data_[index];
  }

  size_t count() const {
    return count_;
  }

  const item_type * begin() const {
    return data_;
  }

  const item_type * end() const {
    return data_ + count_;
  }

  void operator = (parser_vector && origin){
    data_ = origin.data_;
    size_ = origin.size_;
    count_ = origin.count_;
    origin.data_ = nullptr;
  }

private:
  item_type * data_;
  size_t size_;
  size_t count_;
};


#endif //VDS_PARSER_VECTOR_H
