#ifndef __VDS_DATA_CHUNK_H_
#define __VDS_DATA_CHUNK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <assert.h>
#include <vector>

#include "gf.h"
#include "binary_serialize.h"
#include "vds_debug.h"

namespace vds {
    template<typename cell_type>
    class chunk_generator;

    template<typename cell_type>
    class chunk_restore;

    template<typename cell_type>
    class chunk
    {
    public:
        chunk(cell_type k, cell_type n, const std::vector<cell_type> & data);
        chunk(
          const chunk_generator<cell_type> & generator,
          const cell_type * data,
          size_t len);
        ~chunk();

        const std::vector<cell_type> & data() const
        {
          return this->data_;
        }
        static gf_math<cell_type> & math()
        {
          return math_;
        }
        
    private:
        cell_type k_;
        cell_type n_;
        std::vector<cell_type> data_;

        static gf_math<cell_type> math_;
        static void generate_multipliers(
          cell_type * multipliers,
          cell_type k,
          cell_type n);

        friend class chunk_generator<cell_type>; 
        friend class chunk_restore<cell_type>;
    };

    template<typename cell_type>
    class chunk_generator
    {
    public:
        chunk_generator(cell_type k, cell_type n);
        ~chunk_generator();

        const cell_type * multipliers() const
        {
          return this->multipliers_;
        }

        void write(binary_serializer & s, const void * data, size_t size);
        
    private:        
        cell_type k_;
        cell_type n_;
        cell_type * multipliers_;

        friend class chunk<cell_type>;
    };

    template<typename cell_type>
    class chunk_restore
    {
    public:
        chunk_restore(cell_type k, const cell_type * n);
        ~chunk_restore();

        void restore(
          std::vector<cell_type> & result,
          const chunk<cell_type> ** chunks
        );
            
        const_data_buffer restore(
          const std::vector<const_data_buffer> & chunks);
        
        const cell_type * multipliers() const
        {
          return this->multipliers_;
        }
        
    private:
        cell_type k_;
        cell_type * multipliers_;
    };
}

template<typename cell_type>
vds::gf_math<cell_type> vds::chunk<cell_type>::math_;

template<typename cell_type>
void vds::chunk<cell_type>::generate_multipliers(
  cell_type * multipliers,
  cell_type k,
  cell_type n)
{
    cell_type value = 1;
    for (cell_type i = 0; i < k; ++i) {
        *multipliers++ = value;
        value = math_.mul(value, n);
    }
}


template<typename cell_type>
vds::chunk<cell_type>::chunk(
  cell_type k,
  cell_type n,
  const std::vector<cell_type> & data)
    : k_(k), n_(n), data_(data)
{
}

template<typename cell_type>
vds::chunk<cell_type>::chunk(
  const chunk_generator<cell_type> & generator,
  const cell_type * data,
  size_t len)
    : k_(generator.k_), n_(generator.n_)
{
    for (size_t i = 0; i < len; i += this->k_) {
        cell_type value = 0;
        for (cell_type j = 0; j < this->k_; ++j) {
            if (i + j < len) {
                value = math_.add(
                  value,
                  math_.mul(generator.multipliers_[j], data[i + j]));
            }
        }
        this->data_.push_back(value);
    }
}


template<typename cell_type>
vds::chunk<cell_type>::~chunk()
{
}

template<typename cell_type>
vds::chunk_generator<cell_type>::chunk_generator(cell_type k, cell_type n)
    : k_(k), n_(n), multipliers_(new cell_type[k])
{
    chunk<cell_type>::generate_multipliers(this->multipliers_, k, n);
}

template<typename cell_type>
inline vds::chunk_generator<cell_type>::~chunk_generator()
{
  delete[] this->multipliers_;
}

template<typename cell_type>
inline void vds::chunk_generator<cell_type>::write(binary_serializer & s, const void * data, size_t size)
{
  uint64_t expected_size = ((size + sizeof(cell_type) * this->k_ - 1)/ sizeof(cell_type) / this->k_) * sizeof(cell_type);
  auto start = s.size();

  for (size_t i = 0; i < size; i += sizeof(cell_type) * this->k_) {
    cell_type value = 0;
    for (cell_type j = 0; j < this->k_; ++j) {
      cell_type data_item = 0;
      for (size_t offset = 0; offset < sizeof(cell_type); ++offset) {
        data_item <<= 8;
        if (i + sizeof(cell_type) * j + offset < size) {
          data_item |= ((const uint8_t *)data)[i + sizeof(cell_type) * j + offset];
        }
      }
      
      value = chunk<cell_type>::math_.add(
        value,
        chunk<cell_type>::math_.mul(this->multipliers_[j], data_item));
    }

    s << value;
  }
  
  auto final = s.size();
  assert(expected_size == final - start);

  s << safe_cast<uint16_t>(size % (sizeof(cell_type) * this->k_));//Padding
}


template<typename cell_type>
vds::chunk_restore<cell_type>::chunk_restore(cell_type k, const cell_type * n)
: k_(k), multipliers_(new cell_type[k * k])
{
    cell_type * left = new cell_type[k * k];

    //prepare
    auto l = left;
    auto m = this->multipliers_;
    for (cell_type i = 0; i < k; ++i) {
        chunk<cell_type>::generate_multipliers(l, k, n[i]);
        l += k;

        for (cell_type j = 0; j < k; ++j) {
            *m++ = ((i == j) ? 1 : 0);
        }
    }

    //first
    for (cell_type i = 0; i < k; ++i) {
        cell_type m1 = left[k * i + i];
        for (cell_type j = i + 1; j < k; ++j) {
            cell_type m2 = left[k * j + i];
            for (cell_type c = 0; c < k; ++c) {
                if(c < i){
                  assert(left[k * j + c] == 0);
                }
                left[k * j + c] = chunk<cell_type>::math_.sub(
                    chunk<cell_type>::math_.mul(m1, left[k * j + c]),
                    chunk<cell_type>::math_.mul(m2, left[k * i + c])
                    );
                if(c <= i){
                  assert(left[k * j + c] == 0);
                }
                this->multipliers_[k * j + c] = chunk<cell_type>::math_.sub(
                    chunk<cell_type>::math_.mul(m1, this->multipliers_[k * j + c]),
                    chunk<cell_type>::math_.mul(m2, this->multipliers_[k * i + c]));
            }
        }
    }

    m = this->multipliers_;
    for (cell_type i = 0; i < k; ++i) {
      cell_type m1 = left[k * i + i];
      for (cell_type c = 0; c < k; ++c) {
        if (c < i) {
          if (left[k * i + c] != 0) {
            throw std::runtime_error("Logic error 23");
          }
        }
        else {
          left[k * i + c] = chunk<cell_type>::math_.div(left[k * i + c], m1);
        }

        *m = chunk<cell_type>::math_.div(*m, m1);
        ++m;
      }
    }

    //reverse
    for (cell_type i = k; i > 0; --i) {
      for (cell_type j = i - 1; j > 0; --j) {
        cell_type m1 = left[k * (j - 1) + (i - 1)];
        for (cell_type c = 0; c < k; ++c) {
          left[k * (j - 1) + c] = chunk<cell_type>::math_.sub(
            left[k * (j - 1) + c],
            chunk<cell_type>::math_.mul(left[k * (i - 1) + c], m1));
          this->multipliers_[k * (j - 1) + c] = chunk<cell_type>::math_.sub(
            this->multipliers_[k * (j - 1) + c],
            chunk<cell_type>::math_.mul(this->multipliers_[k * (i - 1) + c], m1));
        }
      }
    }

    //validate
    for (cell_type i = 0; i < k; ++i) {
      for (cell_type c = 0; c < k; ++c) {
        auto value = left[k * i + c];
        if(i == c){
          if(1 != value) {
            throw std::runtime_error("Logic error 24");
          }
        }
        else {
          if(0 != value) {
            throw std::runtime_error("Logic error 25");
          }
        }
      }
    }
   delete[] left;
}

template<typename cell_type>
inline vds::chunk_restore<cell_type>::~chunk_restore()
{
  delete[] this->multipliers_;
}

template<typename cell_type>
inline void vds::chunk_restore<cell_type>::restore(
  std::vector<cell_type>& result,
  const chunk<cell_type>** chunks)
{
  for (size_t index = 0; index < chunks[0]->data_.size(); ++index) {
    auto m = this->multipliers_;
    for (cell_type i = 0; i < this->k_; ++i) {
      cell_type value = 0;
      for (cell_type j = 0; j < this->k_; ++j) {
        value = chunk<cell_type>::math_.add(value, 
          chunk<cell_type>::math_.mul(
            *m++, chunks[j]->data_[index]));
      }
      result.push_back(value);
    }
  }
}

template<typename cell_type>
inline vds::const_data_buffer vds::chunk_restore<cell_type>::restore(
  const std::vector<const_data_buffer> & chunks)
{
  binary_serializer s;
  auto size = chunks.begin()->size();
  auto padding = uint16_t(chunks.begin()->data()[size - 2] << 8) | chunks.begin()->data()[size - 1];

  for (cell_type j = 1; j < this->k_; ++j) {
    vds_assert(size == chunks[j].size());
    vds_assert(padding == (uint16_t(chunks[j].data()[size - 2] << 8) | chunks[j].data()[size - 1]));
  }

  auto expected_size = (size - 2) * this->k_;
  if(0 != padding) {
    expected_size -= this->k_ * sizeof(cell_type);
    expected_size += padding;
  }

  for (size_t index = 0; index < size; index += sizeof(cell_type)) {
    auto m = this->multipliers_;
    for (cell_type i = 0; i < this->k_; ++i) {
      cell_type value = 0;
      for (cell_type j = 0; j < this->k_; ++j) {
        cell_type cell = 0;
        for(size_t offset = 0; offset < sizeof(cell_type); ++offset){
          cell <<= 8;
          cell |= chunks[j][index + offset];
        }
        value = chunk<cell_type>::math_.add(value, 
            chunk<cell_type>::math_.mul(
              *m++,
              cell));
      }
      s << value;
      if(s.size() >= expected_size) {
        return const_data_buffer(s.get_buffer(), expected_size);
      }
    }
  }

  throw std::runtime_error("Fatal error at chunk_restore::restore");
}

#endif//__VDS_DATA_CHUNK_H_
