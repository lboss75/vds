#ifndef __VDS_CRYPTO_SSL_TUNNEL_P_H_
#define __VDS_CRYPTO_SSL_TUNNEL_P_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "ssl_tunnel.h"
#include "crypto_exception.h"

namespace vds {
  class certificate;
  class asymmetric_private_key;

  class _ssl_tunnel {
  public:
    _ssl_tunnel(
      ssl_tunnel * owner,
      bool is_client,
      const certificate * cert,
      const asymmetric_private_key * key
    );

    ~_ssl_tunnel();
    
    bool is_client() const {
      return this->is_client_;
    }

    certificate get_peer_certificate() const;

    void set_async_push_crypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t readed)> & handler)
    {
      this->async_push_crypted_handler_ = handler;
    }

    void push_crypted(
      const vds::service_provider & sp,
      const uint8_t * buffer,
      size_t buffer_size)
    {
      this->crypted_input_data_ = buffer;
      this->crypted_input_data_size_ = buffer_size;

      this->start_work_circle(sp);
    }

    void set_async_get_crypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t written)> & handler)
    {
      this->async_get_crypted_handler_ = handler;
    }

    void get_crypted(
      const vds::service_provider & sp,
      uint8_t * buffer,
      size_t buffer_size)
    {
      this->crypted_output_data_ = buffer;
      this->crypted_output_data_size_ = buffer_size;

      this->start_work_circle(sp);
    }

    void set_async_push_decrypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t readed)> & handler)
    {
      this->async_push_decrypted_handler_ = handler;
    }

    void push_decrypted(
      const vds::service_provider & sp,
      const uint8_t * buffer,
      size_t buffer_size)
    {
      this->decrypted_input_data_ = buffer;
      this->decrypted_input_data_size_ = buffer_size;

      this->start_work_circle(sp);
    }

    void set_async_get_decrypted_handler(
      const std::function<void(const vds::service_provider & sp, size_t written)> & handler)
    {
      this->async_get_decrypted_handler_ = handler;
    }

    void get_decrypted(
      const vds::service_provider & sp,
      uint8_t * buffer,
      size_t buffer_size)
    {
      this->decrypted_output_data_ = buffer;
      this->decrypted_output_data_size_ = buffer_size;

      this->start_work_circle(sp);
    }


  private:
    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * input_bio_;
    BIO * output_bio_;

    ssl_tunnel * owner_;
    bool is_client_;

    uint8_t * crypted_output_data_;
    size_t crypted_output_data_size_;

    const uint8_t * crypted_input_data_;
    size_t crypted_input_data_size_;


    uint8_t * decrypted_output_data_;
    size_t decrypted_output_data_size_;

    const uint8_t * decrypted_input_data_;
    size_t decrypted_input_data_size_;

    std::mutex work_circle_mutex_;
    bool work_circle_continue_;
    bool work_circle_started_;

    std::function<void(const vds::service_provider & sp, size_t readed)> async_push_crypted_handler_;
    std::function<void(const vds::service_provider & sp, size_t written)> async_get_crypted_handler_;
    std::function<void(const vds::service_provider & sp, size_t readed)> async_push_decrypted_handler_;
    std::function<void(const vds::service_provider & sp, size_t written)> async_get_decrypted_handler_;

    void start_work_circle(const service_provider & sp)
    {
      this->work_circle_mutex_.lock();
      if (!this->work_circle_started_) {
        imt_service::async(sp, [this, sp]() {
          this->work_circle(sp);
        });
      }
      else {
        this->work_circle_continue_ = true;
      }

      this->work_circle_mutex_.unlock();

    }

    void work_circle(const service_provider & sp)
    {
      for (;;) {
        this->work_circle_mutex_.lock();
        this->work_circle_continue_ = false;
        this->work_circle_mutex_.unlock();

        if (0 < this->crypted_input_data_size_) {
          int bytes = BIO_write(this->input_bio_, this->crypted_input_data_, (int)this->crypted_input_data_size_);
          if (bytes <= 0) {
            if (!BIO_should_retry(this->input_bio_)) {
              throw std::runtime_error("BIO_write failed");
            }
          }
          else {
            this->crypted_input_data_size_ = 0;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->async_push_crypted_handler_(sp, (size_t)bytes);
          }
        }

        if (0 < this->decrypted_input_data_size_) {
          int bytes = SSL_write(this->ssl_, this->decrypted_input_data_, (int)this->decrypted_input_data_size_);
          if (0 > bytes) {
            int ssl_error = SSL_get_error(this->ssl_, bytes);
            switch (ssl_error) {
            case SSL_ERROR_NONE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
              break;
            default:
              throw crypto_exception("SSL_write", ssl_error);
            }
          }
          else {
            this->decrypted_input_data_size_ = 0;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->async_push_decrypted_handler_(sp, (size_t)bytes);
          }
        }

        if (0 < decrypted_output_data_size_) {
          int bytes = SSL_read(this->ssl_, this->decrypted_output_data_, (int)this->decrypted_output_data_size_);
          if (bytes <= 0) {
            int ssl_error = SSL_get_error(this->ssl_, bytes);
            switch (ssl_error) {
            case SSL_ERROR_NONE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_ZERO_RETURN:
              break;
            default:
              throw crypto_exception("BIO_read", ssl_error);
            }
          }
          else {
            this->decrypted_output_data_size_ = 0;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->async_get_decrypted_handler_(sp, (size_t)bytes);
          }
        }

        if (0 < this->crypted_output_data_size_ && BIO_pending(this->output_bio_)) {
          int bytes = BIO_read(this->output_bio_, this->crypted_output_data_, (int)this->crypted_output_data_size_);
          if (bytes <= 0) {
            int ssl_error = SSL_get_error(this->ssl_, bytes);
            switch (ssl_error) {
            case SSL_ERROR_NONE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
              break;
            default:
              throw crypto_exception("BIO_read", ssl_error);
            }
          }
          else {
            this->crypted_output_data_size_ = 0;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->async_get_crypted_handler_(sp, (int)bytes);
          }
        }

        this->work_circle_mutex_.lock();
        if (!this->work_circle_continue_) {
          this->work_circle_started_ = false;
          this->work_circle_mutex_.unlock();
          break;
        }
        this->work_circle_mutex_.unlock();
      }
    }

    void input_stream_processed(const service_provider & sp);
    void output_stream_processed(const service_provider & sp);
  };
}

#endif//__VDS_CRYPTO_SSL_TUNNEL_H_
