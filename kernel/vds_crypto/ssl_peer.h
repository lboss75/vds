#ifndef __VDS_CRYPTO_SSL_PEER_H_
#define __VDS_CRYPTO_SSL_PEER_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class certificate;
  class asymmetric_private_key;

  class flush_output_handler
  {
  public:
    virtual void flush_output() = 0;
  };

  class flush_output_done_handler
  {
  public:
    virtual void flush_output_done() = 0;
  };

  class ssl_peer {
  public:
    ssl_peer(
      bool is_client,
      const certificate * cert,
      const asymmetric_private_key * key
    );

    bool is_init_finished();
    
    size_t write_input(const void * data, size_t len);
    size_t read_output(uint8_t * data, size_t len);

    size_t read_decoded(uint8_t * data, size_t len);
    size_t write_decoded(const void * data, size_t len);

    void flush_output() { this->flush_output_handler_->flush_output(); }
    void set_flush_output_handler(flush_output_handler * handler) { this->flush_output_handler_ = handler; }
    void set_flush_output_done_handler(flush_output_done_handler * handler) { this->flush_output_done_handler_ = handler; }
    void finish_flush_output() { this->flush_output_done_handler_->flush_output_done(); }
    
    bool is_client() const {
      return this->is_client_;
    }

  private:
    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * input_bio_;
    BIO * output_bio_;

    bool is_client_;

    flush_output_handler * flush_output_handler_;
    flush_output_done_handler * flush_output_done_handler_;
  };

  class ssl_input_stream
  {
  public:
    ssl_input_stream(ssl_peer & peer)
      : peer_(peer)
    {
    }

    template < typename context_type >
    class handler
      : public sequence_step<context_type, void(void *, size_t)>,
      private flush_output_done_handler

    {
      using base_class = sequence_step<context_type, void(void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_input_stream & args
      ) : base_class(context),
        peer_(args.peer_),
        data_(nullptr),
        len_(0),
        eof_(false)
      {
        this->peer_.set_flush_output_done_handler(this);
      }

      void operator()(const void * data, size_t len)
      {
        if(0 < this->len_){
          throw new std::runtime_error("State error");
        }
        if (0 == len) {
          this->eof_ = true;
        }

        this->data_ = data;
        this->len_ = len;

        this->processed();
      }

      void processed()
      {
        if (0 < this->len_) {
          auto bytes = this->peer_.write_input(this->data_, this->len_);

          this->data_ = reinterpret_cast<const uint8_t *>(this->data_) + bytes;
          this->len_ -= bytes;
        }

        auto bytes = this->peer_.read_decoded(this->buffer_, BUFFER_SIZE);
        if (bytes > 0) {
          this->next(this->buffer_, bytes);
        }
        else if (this->eof_) {
          this->next(nullptr, 0);
        }
        else {
          this->peer_.flush_output();
        }        
      }

      void flush_output_done() override
      {
        this->prev();
      }

    private:
      ssl_peer & peer_;

      const void * data_;
      size_t len_;

      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];

      bool eof_;
    };
  private:
    ssl_peer & peer_;
  };

  class ssl_output_stream
  {
  public:
    ssl_output_stream(ssl_peer & peer)
      : peer_(peer)
    {
    }

    template < typename context_type >
    class handler
      : public sequence_step<context_type, void(void *, size_t)>,
      private flush_output_handler
    {
      using base_class = sequence_step<context_type, void(void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_output_stream & args
      )
        : base_class(context),
        peer_(args.peer_),
        data_(nullptr),
        len_(0),
        is_passthrough_(false),
        eof_(false)
      {
        this->peer_.set_flush_output_handler(this);
      }

      void operator()(const void * data, size_t len)
      {
        if(this->is_passthrough_ || 0 < this->len_){
          throw new std::runtime_error("State error");
        }
        
        if (0 == len) {
          this->eof_ = true;
        }
        
        this->data_ = data;
        this->len_ = len;
        this->processed();
      }

      void processed()
      {
        if(this->peer_.is_init_finished()) {
          if(0 < this->len_){
            auto written = this->peer_.write_decoded(this->data_, this->len_);
            this->data_ = reinterpret_cast<const uint8_t *>(this->data_) + written;
            this->len_ -= written;
          }
        }
        auto len = this->peer_.read_output(this->buffer_, BUFFER_SIZE);
        if (0 == len) {
          if (this->is_passthrough_) {
            this->is_passthrough_ = false;
            this->peer_.finish_flush_output();
          }
          else if (this->eof_) {
            this->next(nullptr, 0);
          }
          else {
            this->prev();
          }
        }
        else {
          this->next(this->buffer_, len);
        }
      }

      void flush_output() override
      {
        this->is_passthrough_ = true;
        auto len = this->peer_.read_output(this->buffer_, BUFFER_SIZE);
        if (0 == len) {
          this->is_passthrough_ = false;
          this->peer_.finish_flush_output();
        }
        else {
          this->next(this->buffer_, len);
        }
      }

    private:
      ssl_peer & peer_;
      const void * data_;
      size_t len_;
      bool is_passthrough_;
      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];
      bool eof_;
    };

  private:
    ssl_peer & peer_;
  };

}

#endif//__VDS_CRYPTO_SSL_PEER_H_
