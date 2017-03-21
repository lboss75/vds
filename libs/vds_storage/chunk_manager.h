#ifndef __VDS_STORAGE_CHUNK_MANAGER_H_
#define __VDS_STORAGE_CHUNK_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _chunk_manager;

  class chunk_manager
  {
  public:
    chunk_manager(const service_provider & sp);
    ~chunk_manager();

    uint64_t add(const filename & fn);
    uint64_t add(const file_container & container);

  private:
    _chunk_manager * impl_;
  };

  template <typename context_type>
  class chunk_manager_writer : public sequence_step<context_type, void(void)>
  {
    using base_class = sequence_step<context_type, void(void)>;
  public:
    chunk_manager_writer(
      const context_type& context,
      chunk_manager & target,
      uint64_t stream_id)
      : base_class(context),
      target_(target),
      stream_id_(stream_id)
    {
    }

    void operator()(const void * data, size_t len)
    {
      if (0 == len) {
        this->target_.finish_stream(this->stream_id_);
      }
      else {
        this->target_.add_stream(this->stream_id_, data, len);
      }
    }

  private:
    chunk_manager & target_;
    const uint64_t stream_id_;
  };
  
}

#endif // __VDS_STORAGE_CHUNK_MANAGER_H_
