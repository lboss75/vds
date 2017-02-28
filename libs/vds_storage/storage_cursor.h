#ifndef __VDS_STORAGE_STORAGE_CURSOR_H_
#define __VDS_STORAGE_STORAGE_CURSOR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {

  template <typename data_type>
  class storage_cursor;

  //{
  //public:
  //  bool read();

  //  const data_type & current();
  //};

  template <typename data_type>
  class _simple_storage_cursor
  {
  public:
    _simple_storage_cursor(const std::list<data_type> & data)
      : bof_(true), data_(data)
    {
    }

    bool read()
    {
      if (this->bof_) {
        this->position_ = this->data_.begin();
        this->bof_ = false;
      }
      else if (this->data_.end() != this->position_) {
        ++this->position_;
      }

      return this->data_.end() != this->position_;
    }

    const data_type & current() const
    {
      return *this->position_;
    }

  private:
    bool bof_;
    std::list<data_type> data_;
    typename std::list<data_type>::const_iterator position_;
  };
}

#endif // __VDS_STORAGE_STORAGE_CURSOR_H_
