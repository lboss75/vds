#ifndef __VDS_DATA_CHUNK_GENERATE_H_
#define __VDS_DATA_CHUNK_GENERATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <assert.h>
#include "gf.h"

namespace vds {
  template<typename cell_type>
  class chunk_generate
  {
  public:
    chunk_generate(int n, int k)
      : n_(n), k_(k)
    {
    }

    template <
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
    >
      class handler
    {
    public:
      handler(
        done_method_type & done_method,
        next_method_type & next_method,
        error_method_type & error_method,
        const chunk_generate & args)
        : done_method_(done_method),
        next_method_(next_method),
        error_method_(error_method),
        value_(0), value_offset_(0),
        final_sent_(false),
        n_(args.n_), k_(args.k_)
      {
        cell_type value = 1;
        for (int i = 0; i < k_; ++i) {
          this->multipliers_.push_back(value);
          value = this->math_.mul(value, n_);
        }
      }

      void operator()(const void * data, size_t len)
      {
        //Stream final
        if (0 == len) {
          if (0 < this->value_offset_) {
            this->data_.push_back(this->value_);
          }

          if (!this->data_.empty()) {
            this->final_sent_ = true;
            this->next_method_(this->data_.data(), this->data_.size());
          }
          else {
            this->next_method_(nullptr, 0);
          }
        }
        else {
          while (0 < len) {
            auto p = (const char *)data;

            this->value_ = math_.add(
              this->value_,
              math_.mul(this->multipliers_[this->value_offset_++], *p++));

            if (this->k_ == this->value_offset_) {
              this->data_.push_back(this->value_);
              this->value_ = 0;
              this->value_offset_ = 0;
            }

            data = p + 1;
            --len;
          }

          if (!this->data_.empty()) {
            this->next_method_(this->data_.data(), this->data_.size());
          }
          else {
            this->done_method_();
          }
        }
      }

      void processed()
      {
        this->data_.clear();

        if (this->final_sent_) {
          this->final_sent_ = false;
          this->next_method_(nullptr, 0);
        }
        else {
          this->done_method_();
        }
      }

    private:
      done_method_type & done_method_;
      next_method_type & next_method_;
      error_method_type & error_method_;

      bool final_sent_;

      std::vector<cell_type> data_;
      cell_type value_;
      int value_offset_;

      int k_;
      int n_;
      std::vector<cell_type> multipliers_;

      static gf_math<cell_type> math_;
    };

    int k_;
    int n_;
  };
}

template<typename cell_type>
vds::gf_math<cell_type> vds::chunk_generate<cell_type>::math_;


#endif//__VDS_DATA_CHUNK_GENERATE_H_
