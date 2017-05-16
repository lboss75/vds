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
    
    bool is_client() const;

    void set_async_push_crypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t readed)> & handler);

    void push_crypted(
      const vds::service_provider & sp,
      const uint8_t * buffer,
      size_t buffer_size);

    void set_async_get_crypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t written)> & handler);

    void get_crypted(
      const vds::service_provider & sp,
      uint8_t * buffer,
      size_t buffer_size);

    void set_async_push_decrypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t readed)> & handler);

    void push_decrypted(
      const vds::service_provider & sp,
      const uint8_t * buffer,
      size_t buffer_size);

    void set_async_get_decrypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t written)> & handler);

    void get_decrypted(
      const vds::service_provider & sp,
      uint8_t * buffer,
      size_t buffer_size);

  private:
    _ssl_tunnel * const impl_;

  };

  class ssl_tunnel_input_crypted
  {
  public:
    ssl_tunnel_input_crypted(ssl_tunnel & tunnel)
    : tunnel_(tunnel)
    {
    }

    using incoming_item_type = int;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;

    template<typename context_type>
    class handler : public async_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const ssl_tunnel_input_crypted & args)
      : base_class(context),
        tunnel_(args.tunnel_)
      {
        this->tunnel_.set_async_push_crypted_handler(
          [this](const vds::service_provider & sp, size_t readed) {
          this->processed(sp, readed);
        });
      }

      void async_push_data(const vds::service_provider & sp)
      {
        this->tunnel_.push_crypted(
          sp,
          this->input_buffer_,
          this->input_buffer_size_);
      }

    private:
      ssl_tunnel & tunnel_;
    };

  private:
    ssl_tunnel & tunnel_;
  };

  class ssl_tunnel_output_crypted
  {
  public:
    ssl_tunnel_output_crypted(ssl_tunnel & tunnel)
      : tunnel_(tunnel)
    {
    }

    using outgoing_item_type = int;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public async_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const ssl_tunnel_output_crypted & args)
        : base_class(context),
        tunnel_(args.tunnel_)
      {
        this->tunnel_.set_async_get_crypted_handler(
          [this](const vds::service_provider & sp, size_t written) {
          this->processed(sp, written);
        });
      }

      void async_get_data(const vds::service_provider & sp)
      {
        this->tunnel_.get_crypted(
          sp,
          this->output_buffer_,
          this->output_buffer_size_);
      }

    private:
      ssl_tunnel & tunnel_;
    };

  private:
    ssl_tunnel & tunnel_;
  };

  class ssl_tunnel_input_decrypted
  {
  public:
    ssl_tunnel_input_decrypted(ssl_tunnel & tunnel)
      : tunnel_(tunnel)
    {
    }

    using incoming_item_type = int;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;

    template<typename context_type>
    class handler : public async_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const ssl_tunnel_input_decrypted & args)
      : base_class(context),
        tunnel_(args.tunnel_)
      {
        this->tunnel_.set_async_push_decrypted_handler(
          [this](const vds::service_provider & sp, size_t readed) {
          this->processed(sp, readed);
        });
      }

      void async_push_data(const vds::service_provider & sp)
      {
        this->tunnel_.push_decrypted(
          sp,
          this->input_buffer_,
          this->input_buffer_size_);
      }

    private:
      ssl_tunnel & tunnel_;
    };

  private:
    ssl_tunnel & tunnel_;
  };

  class ssl_tunnel_output_decrypted
  {
  public:
    ssl_tunnel_output_decrypted(ssl_tunnel & tunnel)
      : tunnel_(tunnel)
    {
    }

    using outgoing_item_type = int;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public async_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const ssl_tunnel_output_decrypted & args)
        : base_class(context),
        tunnel_(args.tunnel_)
      {
        this->tunnel_.set_async_get_decrypted_handler(
          [this](const vds::service_provider & sp, size_t written) {
          this->processed(sp, written);
        });
      }

      void async_get_data(const vds::service_provider & sp)
      {
        this->tunnel_.get_decrypted(
          sp,
          this->output_buffer_,
          this->output_buffer_size_);
      }

    private:
      ssl_tunnel & tunnel_;
    };

  private:
    ssl_tunnel & tunnel_;
  };
}

#endif//__VDS_CRYPTO_SSL_TUNNEL_H_
