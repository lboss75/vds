//#ifndef __VDS_CRYPTO_SSL_TUNNEL_P_H_
//#define __VDS_CRYPTO_SSL_TUNNEL_P_H_
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "ssl_tunnel.h"
//#include "crypto_exception.h"
//
//namespace vds {
//  class certificate;
//  class asymmetric_private_key;
//
//  class _ssl_tunnel {
//  public:
//    _ssl_tunnel(
//      const service_provider & sp,
//      const stream_async<uint8_t> & crypted_output,
//      const stream_async<uint8_t> & decrypted_output,
//      bool is_client,
//      const certificate * cert,
//      const asymmetric_private_key * key);
//
//    ~_ssl_tunnel();
//    
//    bool is_client() const {
//      return this->is_client_;
//    }
//
//    certificate get_peer_certificate() const;
//
//    stream_async<uint8_t> & crypted_input()  { return this->crypted_input_; }
//    stream_async<uint8_t> & decrypted_input() { return this->decrypted_input_; }
//
//    void start(const service_provider & sp)
//    {
//      this->start_crypted_input(sp);
//      this->start_decrypted_input(sp);
// 
//      this->process(sp);
//    }
//    
//    void on_error(const std::function<void(const std::shared_ptr<std::exception> &)> & handler)
//    {
//      this->error_handler_ = handler;
//    }
//
//  private:
//    SSL_CTX *ssl_ctx_;
//    SSL * ssl_;
//    BIO * input_bio_;
//    BIO * output_bio_;
//
//    ssl_tunnel * owner_;
//    bool is_client_;
//
//    uint8_t crypted_output_data_[1024];
//    size_t crypted_output_data_size_;
//
//    uint8_t crypted_input_data_[1024];
//    size_t crypted_input_data_size_;
//
//    uint8_t decrypted_output_data_[1024];
//    size_t decrypted_output_data_size_;
//
//    uint8_t decrypted_input_data_[1024];
//    size_t decrypted_input_data_size_;
//
//    bool crypted_input_eof_;
//    bool decrypted_input_eof_;
//
//    std::mutex state_mutex_;
//
//    continuous_buffer<uint8_t> crypted_input_;
//    stream_async<uint8_t> crypted_output_;
//
//    continuous_buffer<uint8_t> decrypted_input_;
//    stream_async<uint8_t> decrypted_output_;
//    
//    bool failed_state_;
//    std::function<void(const std::shared_ptr<std::exception> &)> error_handler_;
//
//    void start_crypted_input(const service_provider & sp)
//    {
//      try
//      {
//        size_t readed = this->crypted_input_.read_async(this->crypted_input_data_, sizeof(this->crypted_input_data_)).get();
//        this->state_mutex_.lock();
//
//        if (0 < readed) {
//          this->crypted_input_data_size_ = readed;
//        }
//        else {
//          this->crypted_input_eof_ = true;
//          sp.get<logger>()->trace("SSL", sp, "SSL Crypted input closed");
//        }
//
//        this->state_mutex_.unlock();
//        this->process(sp);
//      }
//      catch (const std::exception & ex) {
//        this->state_mutex_.lock();
//
//        if (this->failed_state_) {
//          this->failed_state_ = true;
//          this->state_mutex_.unlock();
//
//          this->error_handler_(std::make_shared<std::runtime_error>(ex.what()));
//        }
//        else {
//          this->state_mutex_.unlock();
//        }
//      }
//    }
//    
//    void start_decrypted_input(const service_provider & sp)
//    {
//      if(this->decrypted_input_eof_){
//        throw std::runtime_error("Login error");
//      }
//      
//      try {
//        size_t readed = this->decrypted_input_.read_async(this->decrypted_input_data_, sizeof(this->decrypted_input_data_)).get();
//        this->state_mutex_.lock();
//        if (0 < readed) {
//          this->decrypted_input_data_size_ = readed;
//        }
//        else {
//          this->decrypted_input_eof_ = true;
//          sp.get<logger>()->trace("SSL", sp, "SSL Decrypted input closed");
//        }
//
//        this->state_mutex_.unlock();
//
//        this->process(sp);
//      }
//      catch (const std::exception & ex) {
//        this->state_mutex_.lock();
//
//        if (this->failed_state_) {
//          this->failed_state_ = true;
//          this->state_mutex_.unlock();
//
//          this->error_handler_(std::make_shared<std::runtime_error>(ex.what()));
//        }
//        else {
//          this->state_mutex_.unlock();
//        }
//      }
//    }
//
//    void process(const service_provider & sp)
//    {
//      std::unique_lock<std::mutex> lock(this->state_mutex_);
//
//        if (0 < this->crypted_input_data_size_) {
//          int bytes = BIO_write(this->input_bio_, this->crypted_input_data_, (int)this->crypted_input_data_size_);
//          if (bytes <= 0) {
//            if (!BIO_should_retry(this->input_bio_)) {
//              throw std::runtime_error("BIO_write failed");
//            }
//          }
//          else {
//            assert((size_t)bytes == this->crypted_input_data_size_);
//            this->crypted_input_data_size_ = 0;
//            this->start_crypted_input(sp);
//          }
//        }
//
//        if (0 < this->decrypted_input_data_size_) {
//          int bytes = SSL_write(this->ssl_, this->decrypted_input_data_, (int)this->decrypted_input_data_size_);
//          if (0 > bytes) {
//            int ssl_error = SSL_get_error(this->ssl_, bytes);
//            switch (ssl_error) {
//            case SSL_ERROR_NONE:
//            case SSL_ERROR_WANT_READ:
//            case SSL_ERROR_WANT_WRITE:
//            case SSL_ERROR_WANT_CONNECT:
//            case SSL_ERROR_WANT_ACCEPT:
//              break;
//            default:
//              throw crypto_exception("SSL_write", ssl_error);
//            }
//          }
//          else {
//            assert((size_t)bytes == this->decrypted_input_data_size_);
//            
//            this->decrypted_input_data_size_ = 0;
//            this->start_decrypted_input(sp);
//          }
//        }
//
//        if (0 == decrypted_output_data_size_) {
//          int bytes = SSL_read(this->ssl_, this->decrypted_output_data_, (int)sizeof(this->decrypted_output_data_));
//          if (bytes <= 0) {
//            int ssl_error = SSL_get_error(this->ssl_, bytes);
//            switch (ssl_error) {
//            case SSL_ERROR_NONE:
//            case SSL_ERROR_WANT_READ:
//            case SSL_ERROR_WANT_WRITE:
//            case SSL_ERROR_WANT_CONNECT:
//            case SSL_ERROR_WANT_ACCEPT:
//            case SSL_ERROR_ZERO_RETURN:
//              if (this->crypted_input_eof_ && this->decrypted_output_) {
//                auto & tmp = this->decrypted_output_;
//                this->decrypted_output_ = continuous_buffer<uint8_t>(sp);
//
//                tmp.write_async(nullptr, 0)
//                  .execute(
//                    [this, sp](const std::shared_ptr<std::exception> & ex) {
//                      if(!ex){
//                        sp.get<logger>()->trace("SSL", sp, "SSL Decrypted output closed");
//                      } else {
//                        this->state_mutex_.lock();
//                        if(this->failed_state_){
//                          this->failed_state_ = true;
//                          this->state_mutex_.unlock();
//                          
//                          this->error_handler_(ex);
//                        }
//                        else {
//                          this->state_mutex_.unlock();
//                        }                      
//                      }
//                    });
//              }
//              break;
//            default:
//              throw crypto_exception("SSL_read", ssl_error);
//            }
//          }
//          else {
//            this->decrypted_output_data_size_ = bytes;
//
//            this->decrypted_output_.write_async(this->decrypted_output_data_, (size_t)bytes)
//              .execute([this, sp](const std::shared_ptr<std::exception> & ex) {
//                  this->state_mutex_.lock();
//                  if(!ex){
//                    this->decrypted_output_data_size_ = 0;
//                    this->state_mutex_.unlock();
//
//                    this->process(sp);
//                  } else {
//                    if(this->failed_state_){
//                      this->failed_state_ = true;
//                      this->state_mutex_.unlock();
//                      
//                      this->error_handler_(ex);
//                    }
//                    else {
//                      this->state_mutex_.unlock();
//                    }
//                  }
//            });
//            return;
//          }
//        }
//
//        if (0 == this->crypted_output_data_size_ && BIO_pending(this->output_bio_)) {
//          int bytes = BIO_read(this->output_bio_, this->crypted_output_data_, (int)sizeof(this->crypted_output_data_));
//          if (bytes <= 0) {
//            int ssl_error = SSL_get_error(this->ssl_, bytes);
//            switch (ssl_error) {
//            case SSL_ERROR_NONE:
//            case SSL_ERROR_WANT_READ:
//            case SSL_ERROR_WANT_WRITE:
//            case SSL_ERROR_WANT_CONNECT:
//            case SSL_ERROR_WANT_ACCEPT:
//              break;
//            default:
//              throw crypto_exception("BIO_read", ssl_error);
//            }
//          }
//          else {
//            this->crypted_output_data_size_ = (size_t)bytes;
//
//            this->crypted_output_.write_async(this->crypted_output_data_, (size_t)bytes)
//              .execute([this, sp](const std::shared_ptr<std::exception> & ex) {
//              this->state_mutex_.lock();
//              if(!ex){
//                this->crypted_output_data_size_ = 0;
//                this->state_mutex_.unlock();
//
//                this->process(sp);
//              } else {
//                if(this->failed_state_){
//                  this->failed_state_ = true;
//                  this->state_mutex_.unlock();
//                  
//                  this->error_handler_(ex);
//                }
//                else {
//                  this->state_mutex_.unlock();
//                }
//              }
//            });
//            return;
//          }
//        }
//
//          if (this->decrypted_input_eof_ 
//            && (this->is_client_ || this->crypted_input_eof_)
//            && 0 == this->crypted_output_data_size_
//            && this->crypted_output_) {
//            auto tmp = this->crypted_output_;
//            this->crypted_output_ = continuous_buffer<uint8_t>(sp);
//
//            tmp.write_async(nullptr, 0)
//              .execute([this, sp](const std::shared_ptr<std::exception> & ex) {
//                if(!ex){
//                  sp.get<logger>()->trace("SSL", sp, "SSL Crypted output closed");
//                } else {
//                  this->state_mutex_.lock();
//                  if(this->failed_state_){
//                    this->failed_state_ = true;
//                    this->state_mutex_.unlock();
//                    
//                    this->error_handler_(ex);
//                  }
//                  else {
//                    this->state_mutex_.unlock();
//                  }
//                }
//              });
//          }
//    }
//          
//  };
//}
//
//#endif//__VDS_CRYPTO_SSL_TUNNEL_H_
