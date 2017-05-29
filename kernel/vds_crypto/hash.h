#ifndef __VDS_CRYPTO_HASH_H_
#define __VDS_CRYPTO_HASH_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "dataflow.h"

namespace vds {
  struct hash_info;

  class _hash;
  class hash
  {
  public:
    static const hash_info & sha256();

    hash(const hash_info & info);
    ~hash();

    void update(
      const void * data,
      size_t len);

    void final();

    const const_data_buffer & signature() const;
    
    static const_data_buffer signature(
      const hash_info & info,
      const const_data_buffer & data);

    static const_data_buffer signature(
      const hash_info & info,
      const void * data,
      size_t data_size);
    
  private:
    _hash * impl_;
  };

  class _hmac;
  class hmac
  {
  public:
    hmac(const std::string & key, const hash_info & info = hash::sha256());
    ~hmac();

    void update(
      const void * data,
      size_t len);

    void final();

    const const_data_buffer signature() const;

  private:
    _hmac * impl_;
  };

  class hash_filter
  {
  public:
    hash_filter(
      size_t * size_result, 
      const_data_buffer * hash_result,
      const hash_info & info = hash::sha256())
      : size_result_(size_result), hash_result_(hash_result), info_(info)
    {
    }

    using incoming_item_type = uint8_t;
    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
    {
    public:
      handler(
        const context_type & context,
        const hash_filter & args)
        : vds::sync_dataflow_filter<context_type, handler<context_type>>(context),
        size_result_(args.size_result_),
        hash_result_(args.hash_result_),
        size_(0),
        hash_(args.info_)
      {
      }

      void sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
      {
        auto n = (this->input_buffer_size() < this->output_buffer_size())
          ? this->input_buffer_size()
          : this->output_buffer_size();

        if (0 == n) {
          *this->size_result_ = this->size_;
          *this->hash_result_ = this->hash_.signature();
        }
        else {
          this->hash_.update(this->input_buffer(), n);
          memcpy(this->output_buffer(), this->input_buffer(), n);

          this->size_ += n;
        }

        input_readed = output_written = n;
      }

    private:
      size_t * size_result_;
      const_data_buffer * hash_result_;

      size_t size_;
      hash hash_;
    };
  private:
    size_t * size_result_;
    const_data_buffer * hash_result_;
    const hash_info & info_;
  };

}

#endif // __HASH_H_
