#ifndef __VDS_DATA_CHUNK_H_
#define __VDS_DATA_CHUNK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <assert.h>
#include "gf.h"

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
          std::vector<cell_type> & multipliers,
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

        const std::vector<cell_type> & multipliers() const
        {
          return this->multipliers_;
        }
        
    private:        
        cell_type k_;
        cell_type n_;
        std::vector<cell_type> multipliers_;

        friend class chunk<cell_type>;
    };

    template<typename cell_type>
    class chunk_restore
    {
    public:
        chunk_restore(cell_type k, const cell_type * n);

        void restore(
          std::vector<cell_type> & result,
          const chunk<cell_type> ** chunks
        );
            
        const std::vector<std::vector<cell_type>> & multipliers() const
        {
          return this->multipliers_;
        }
        
    private:
        cell_type k_;
        std::vector<std::vector<cell_type>> multipliers_;
    };
}

template<typename cell_type>
vds::gf_math<cell_type> vds::chunk<cell_type>::math_;

template<typename cell_type>
void vds::chunk<cell_type>::generate_multipliers(
  std::vector<cell_type> & multipliers,
  cell_type k,
  cell_type n)
{
    cell_type value = 1;
    for (cell_type i = 0; i < k; ++i) {
        multipliers.push_back(value);
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
    : k_(k), n_(n)
{
    chunk<cell_type>::generate_multipliers(this->multipliers_, k, n);
}


template<typename cell_type>
vds::chunk_restore<cell_type>::chunk_restore(cell_type k, const cell_type * n)
: k_(k)
{
    std::vector<std::vector<cell_type>> left;

    //prepare
    for (cell_type i = 0; i < k; ++i) {
        left.push_back(std::vector<cell_type>());
        chunk<cell_type>::generate_multipliers(*left.rbegin(), k, n[i]);

        this->multipliers_.push_back(std::vector<cell_type>());
        for (cell_type j = 0; j < k; ++j) {
            this->multipliers_.rbegin()->push_back((i == j) ? 1 : 0);
        }
    }

    //first
    for (cell_type i = 0; i < k; ++i) {
        cell_type m1 = left.at(i).at(i);
        for (cell_type j = i + 1; j < k; ++j) {
            cell_type m2 = left.at(j).at(i);
            for (cell_type c = 0; c < k; ++c) {
                if(c < i){
                  assert(left.at(j).at(c) == 0);
                }
                left.at(j).at(c) = chunk<cell_type>::math_.sub(
                    chunk<cell_type>::math_.mul(m1, left.at(j).at(c)),
                    chunk<cell_type>::math_.mul(m2, left.at(i).at(c))
                    );
                if(c <= i){
                  assert(left.at(j).at(c) == 0);
                }
                this->multipliers_.at(j).at(c) = chunk<cell_type>::math_.sub(
                    chunk<cell_type>::math_.mul(m1, this->multipliers_.at(j).at(c)),
                    chunk<cell_type>::math_.mul(m2, this->multipliers_.at(i).at(c))
                    );
            }
        }
    }

    //reverse
    for (cell_type i = 0; i < k; ++i) {
        cell_type m0 = left.at(i).at(i);
        assert(m0 != 0);
        for (cell_type j = i + 1; j < k; ++j) {
            cell_type m1 = left.at(i).at(j);
            if(m1 != 0) {
              cell_type m2 = left.at(j).at(j);
              assert(m2 != 0);
              for (cell_type c = 0; c < k; ++c) {
                  left.at(i).at(c) = chunk<cell_type>::math_.div(
                      chunk<cell_type>::math_.sub(
                          chunk<cell_type>::math_.mul(m2, left.at(i).at(c)),
                          chunk<cell_type>::math_.mul(m1, left.at(j).at(c))
                          ),
                          chunk<cell_type>::math_.mul(m2, m0));
                  if(c <= i) {
                    assert(left.at(i).at(c) == ((c == i) ? 1 : 0));
                  }
                  this->multipliers_.at(i).at(c) = chunk<cell_type>::math_.div(
                      chunk<cell_type>::math_.sub(
                          chunk<cell_type>::math_.mul(m2, this->multipliers_.at(i).at(c)),
                          chunk<cell_type>::math_.mul(m1, this->multipliers_.at(j).at(c))
                          ),
                          chunk<cell_type>::math_.mul(m2, m0));
              }
            }
        }
        if (i == k - 1){//fix last row
            for (cell_type c = 0; c < k; ++c) {
                left.at(i).at(c) = chunk<cell_type>::math_.div(left.at(i).at(c),m0);
                if(c <= i) {
                  assert(left.at(i).at(c) == ((c == i) ? 1 : 0));
                }
                this->multipliers_.at(i).at(c) = chunk<cell_type>::math_.div(
                    this->multipliers_.at(i).at(c), m0);
            }
        }
    }
}

template<typename cell_type>
inline void vds::chunk_restore<cell_type>::restore(
  std::vector<cell_type>& result,
  const chunk<cell_type>** chunks)
{
    for (size_t index = 0; index < chunks[0]->data_.size(); ++index) {
        for (cell_type i = 0; i < this->k_; ++i) {
            cell_type value = 0;
            for (cell_type j = 0; j < this->k_; ++j) {
                value = chunk<cell_type>::math_.add(value, 
                    chunk<cell_type>::math_.mul(
                      this->multipliers_.at(i).at(j), chunks[j]->data_[index]));
            }
            result.push_back(value);
        }
    }
}

#endif//__VDS_DATA_CHUNK_H_
