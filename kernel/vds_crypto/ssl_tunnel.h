#ifndef __VDS_CRYPTO_SSL_TUNNEL_H_
#define __VDS_CRYPTO_SSL_TUNNEL_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class certificate;
  class asymmetric_private_key;

  class _ssl_tunnel;
  class ssl_tunnel {
  public:
    ssl_tunnel(
      bool is_client,
      const certificate * cert,
      const asymmetric_private_key * key
    );

    ~ssl_tunnel();
    
    bool is_client() const {
      return this->is_client_;
    }

    certificate get_peer_certificate() const;

  private:
    lifetime_check lt_;
    friend class ssl_input_stream;
    friend class ssl_output_stream;
    friend class _ssl_tunnel;

    class issl_input_stream
    {
    public:
      virtual void input_done(const service_provider & sp) = 0;
      virtual void decoded_output_done(const service_provider & sp, size_t len) = 0;

      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];
    };

    class issl_output_stream
    {
    public:
      virtual void decoded_input_done(const service_provider & sp) = 0;
      virtual void output_done(const service_provider & sp, size_t len) = 0;

      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];
    };


    _ssl_tunnel * impl_;

    bool is_client_;

    const void * input_data_;
    size_t input_len_;

    const void * decoded_input_data_;
    size_t decoded_input_len_;

    issl_input_stream * input_stream_;
    issl_output_stream * output_stream_;

    bool input_stream_done_;
    bool output_stream_done_;

    std::mutex work_circle_mutex_;
    int work_circle_queries_;

    bool enable_output_;

    void set_input_stream(issl_input_stream * stream);
    void set_output_stream(issl_output_stream * stream);

    void write_input(const service_provider & sp, const void * data, size_t len);
    void write_decoded_output(const service_provider & sp, const void * data, size_t len);

    void start_work_circle();
    void work_circle();
    void input_stream_processed(const service_provider & sp);
    void output_stream_processed(const service_provider & sp);
  };

  class ssl_input_stream
  {
  public:
    ssl_input_stream(ssl_tunnel & tunnel)
      : tunnel_(tunnel)
    {
    }

    template < typename context_type >
    class handler
      : public dataflow_step<context_type, bool(const void *, size_t)>,
        public ssl_tunnel::issl_input_stream
    {
      using base_class = dataflow_step<context_type, bool(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_input_stream & args
      );

      ~handler();

      void input_done(const service_provider & sp) override;
      void decoded_output_done(const service_provider & sp, size_t len) override;

      bool operator()(const service_provider & sp, const void * data, size_t len);
      void processed(const service_provider & sp);

    private:
      lifetime_check lt_;
      ssl_tunnel & tunnel_;
    };
  private:
    ssl_tunnel & tunnel_;
  };

  class ssl_output_stream
  {
  public:
    ssl_output_stream(ssl_tunnel & tunnel)
      : tunnel_(tunnel)
    {
    }

    template < typename context_type >
    class handler
      : public dataflow_step<context_type, bool(const void *, size_t)>,
        public ssl_tunnel::issl_output_stream
    {
      using base_class = dataflow_step<context_type, bool(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_output_stream & args
      );

      ~handler();

      void decoded_input_done(const service_provider & sp) override;
      void output_done(const service_provider & sp, size_t len) override;

      bool operator()(const service_provider & sp, const void * data, size_t len);
      void processed(const service_provider & sp);

    private:
      lifetime_check lt_;
      ssl_tunnel & tunnel_;
    };

  private:
    ssl_tunnel & tunnel_;
  };
  
  //scope property
  class peer_certificate
  {
  public:
    peer_certificate(const ssl_tunnel * owner);
    
    certificate get_peer_certificate() const;
    
  private:
    const ssl_tunnel * owner_; 
  };

  template<typename context_type>
  inline ssl_input_stream::handler<context_type>::handler(const context_type & context, const ssl_input_stream & args)
  : base_class(context),
    tunnel_(args.tunnel_)
  {
    this->tunnel_.set_input_stream(this);
  }

  template<typename context_type>
  inline ssl_input_stream::handler<context_type>::~handler()
  {
    this->tunnel_.set_input_stream(nullptr);
  }

  template<typename context_type>
  inline void ssl_input_stream::handler<context_type>::input_done(const service_provider & sp)
  {
    this->prev(sp);
  }

  template<typename context_type>
  inline void ssl_input_stream::handler<context_type>::decoded_output_done(const service_provider & sp, size_t len)
  {
    this->next(sp, this->buffer_, len);
  }

  template<typename context_type>
  inline bool ssl_input_stream::handler<context_type>::operator()(
    const service_provider & sp,
    const void * data,
    size_t len)
  {
    if (0 == len) {
      return this->next(sp, nullptr, 0);
    }
    else {
      this->tunnel_.write_input(sp, data, len);
      return false;
    }
  }

  template<typename context_type>
  inline void ssl_input_stream::handler<context_type>::processed(const service_provider & sp)
  {
    this->tunnel_.input_stream_processed(sp);
  }


  template<typename context_type>
  inline ssl_output_stream::handler<context_type>::handler(const context_type & context, const ssl_output_stream & args)
  : base_class(context),
    tunnel_(args.tunnel_)
  {
    this->tunnel_.set_output_stream(this);
  }

  template<typename context_type>
  inline ssl_output_stream::handler<context_type>::~handler()
  {
    this->tunnel_.set_output_stream(nullptr);
  }

  template<typename context_type>
  inline void ssl_output_stream::handler<context_type>::decoded_input_done(const service_provider & sp)
  {
    this->prev(sp);
  }

  template<typename context_type>
  inline void ssl_output_stream::handler<context_type>::output_done(const service_provider & sp, size_t len)
  {
    this->next(sp, this->buffer_, len);
  }

  template<typename context_type>
  inline bool ssl_output_stream::handler<context_type>::operator()(const service_provider & sp, const void * data, size_t len)
  {
    if (0 == len) {
      return this->next(sp, nullptr, 0);
    }
    else {
      this->tunnel_.write_decoded_output(sp, data, len);
      return false;
    }
  }

  template<typename context_type>
  inline void ssl_output_stream::handler<context_type>::processed(const service_provider & sp)
  {
    this->tunnel_.output_stream_processed(sp);
  }
}

#endif//__VDS_CRYPTO_SSL_TUNNEL_H_
