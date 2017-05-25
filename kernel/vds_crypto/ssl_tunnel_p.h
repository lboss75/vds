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

    std::shared_ptr<async_stream<uint8_t>> crypted_input() const { return this->crypted_input_; }
    std::shared_ptr<async_stream<uint8_t>> crypted_output() const { return this->crypted_output_; }

    std::shared_ptr<async_stream<uint8_t>> decrypted_input() const { return this->decrypted_input_; }
    std::shared_ptr<async_stream<uint8_t>> decrypted_output() const { return this->decrypted_output_; }

    void start(const service_provider & sp)
    {
      this->crypted_input_->read_async(sp, this->crypted_input_data_, sizeof(this->crypted_input_data_))
        .wait([this](const service_provider & sp, size_t readed) {
        if (0 < readed) {
          this->work_circle_mutex_.lock();
          this->crypted_input_data_size_ = readed;
          this->work_circle_mutex_.unlock();

          this->start_work_circle(sp);
        }
        else {
          this->work_circle_mutex_.lock();
          this->crypted_input_eof_ = true;
          this->work_circle_mutex_.unlock();

          this->start_work_circle(sp);
        }
      },
          [this](const service_provider & sp, std::exception_ptr ex) {
      },
        sp);

      this->decrypted_input_->read_async(sp, this->decrypted_input_data_, sizeof(this->decrypted_input_data_))
        .wait([this](const service_provider & sp, size_t readed) {
        if (0 < readed) {
          this->work_circle_mutex_.lock();
          this->decrypted_input_data_size_ = readed;
          this->work_circle_mutex_.unlock();

          this->start_work_circle(sp);
        }
        else {
          this->work_circle_mutex_.lock();
          this->decrypted_input_eof_ = true;
          this->work_circle_mutex_.unlock();

          this->start_work_circle(sp);
        }
      },
          [this](const service_provider & sp, std::exception_ptr ex) {
      },
        sp);

      this->start_work_circle(sp);
    }

  private:
    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * input_bio_;
    BIO * output_bio_;

    ssl_tunnel * owner_;
    bool is_client_;

    uint8_t crypted_output_data_[1024];
    size_t crypted_output_data_size_;

    uint8_t crypted_input_data_[1024];
    size_t crypted_input_data_size_;

    uint8_t decrypted_output_data_[1024];
    size_t decrypted_output_data_size_;

    uint8_t decrypted_input_data_[1024];
    size_t decrypted_input_data_size_;

    bool crypted_input_eof_;
    bool decrypted_input_eof_;

    std::mutex work_circle_mutex_;
    bool work_circle_continue_;
    bool work_circle_started_;

    std::shared_ptr<async_stream<uint8_t>> crypted_input_;
    std::shared_ptr<async_stream<uint8_t>> crypted_output_;

    std::shared_ptr<async_stream<uint8_t>> decrypted_input_;
    std::shared_ptr<async_stream<uint8_t>> decrypted_output_;

    void start_work_circle(const service_provider & sp)
    {
      this->work_circle_mutex_.lock();
      if (!this->work_circle_started_) {
        this->work_circle_started_ = true;
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
            assert(bytes == this->crypted_input_data_size_);
            this->crypted_input_data_size_ = 0;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->crypted_input_->read_async(sp, this->crypted_input_data_, sizeof(this->crypted_input_data_))
              .wait([this](const service_provider & sp, size_t readed) {
                if (0 < readed) {
                  this->work_circle_mutex_.lock();
                  this->crypted_input_data_size_ = readed;
                  this->work_circle_mutex_.unlock();

                  this->start_work_circle(sp);
                }
                else {
                  this->work_circle_mutex_.lock();
                  this->crypted_input_eof_ = true;
                  this->work_circle_mutex_.unlock();

                  this->start_work_circle(sp);
                }
              },
                [this](const service_provider & sp, std::exception_ptr ex) {
              },
                sp);
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
            assert(bytes == this->decrypted_input_data_size_);
            this->decrypted_input_data_size_ = 0;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->decrypted_input_->read_async(sp, this->decrypted_input_data_, sizeof(this->decrypted_input_data_))
              .wait([this](const service_provider & sp, size_t readed) {
              if (0 < readed) {
                this->work_circle_mutex_.lock();
                this->decrypted_input_data_size_ = readed;
                this->work_circle_mutex_.unlock();

                this->start_work_circle(sp);
              }
              else {
                this->work_circle_mutex_.lock();
                this->decrypted_input_eof_ = true;
                this->work_circle_mutex_.unlock();

                this->start_work_circle(sp);
              }
            },
                [this](const service_provider & sp, std::exception_ptr ex) {
            },
              sp);
          }
        }

        if (0 == decrypted_output_data_size_) {
          int bytes = SSL_read(this->ssl_, this->decrypted_output_data_, (int)sizeof(this->decrypted_output_data_));
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
            this->decrypted_output_data_size_ = bytes;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->decrypted_output_->write_all_async(sp, this->decrypted_output_data_, (size_t)bytes)
              .wait([this](const service_provider & sp) {
              this->work_circle_mutex_.lock();
              this->decrypted_output_data_size_ = 0;
              this->work_circle_mutex_.unlock();

              this->start_work_circle(sp);
            },
                [this](const service_provider & sp, std::exception_ptr ex) {
            },
              sp);
          }
        }

        if (0 == this->crypted_output_data_size_ && BIO_pending(this->output_bio_)) {
          int bytes = BIO_read(this->output_bio_, this->crypted_output_data_, (int)sizeof(this->crypted_output_data_));
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
            this->crypted_output_data_size_ = (size_t)bytes;

            this->work_circle_mutex_.lock();
            this->work_circle_continue_ = true;
            this->work_circle_mutex_.unlock();

            this->crypted_output_->write_all_async(sp, this->crypted_output_data_, (size_t)bytes)
              .wait([this](const service_provider & sp) {
              this->work_circle_mutex_.lock();
              this->crypted_output_data_size_ = 0;
              this->work_circle_mutex_.unlock();

              this->start_work_circle(sp);
            },
                [this](const service_provider & sp, std::exception_ptr ex) {
            },
              sp);
          }
        }

        this->work_circle_mutex_.lock();
        if (!this->work_circle_continue_) {
          this->work_circle_started_ = false;

          if (this->decrypted_input_eof_ && this->crypted_output_) {
            auto tmp = this->crypted_output_;
            this->crypted_output_.reset();

            tmp->write_all_async(sp, nullptr, 0)
              .wait([this](const service_provider & sp) {},
                [this](const service_provider & sp, std::exception_ptr ex) {},
                sp);
          }

          if (this->crypted_input_eof_ && this->decrypted_output_) {
            auto tmp = this->decrypted_output_;
            this->decrypted_output_.reset();

            tmp->write_all_async(sp, nullptr, 0)
              .wait(
                [this](const service_provider & sp) {},
                [this](const service_provider & sp, std::exception_ptr ex) {},
                sp);
          }

          this->work_circle_mutex_.unlock();
          break;
        }
        this->work_circle_mutex_.unlock();
      }
    }
  };
}

#endif//__VDS_CRYPTO_SSL_TUNNEL_H_
