#ifndef __VDS_HTTP_HTTPS_PIPELINE_P_H_
#define __VDS_HTTP_HTTPS_PIPELINE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "https_pipeline.h"
#include "http_response.h"
#include "http_incoming_stream.h"
#include "http_request.h"
#include "http_outgoing_stream.h"
#include "http_response_parser.h"
#include "http_request_serializer.h"


namespace vds {
  class _https_pipeline
  {
  public:
    _https_pipeline(
      https_pipeline * owner,
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key);
    
    ~_https_pipeline();
      
    void connect(const service_provider & sp);
    
    const std::string & address() const {
      return this->address_;
    }

    int port() const {
      return this->port_;
    }
    
    void run(const service_provider & sp, const std::string & body);

  private:
    class ioutput_command_stream;

    https_pipeline * owner_;
    std::string address_;
    int port_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;
    ioutput_command_stream * output_command_stream_;
    
    class connection
    {
    public:
      connection(
        _https_pipeline * owner);

      template <typename context_type>
      class handler : public dataflow_step<context_type, bool(void)>
      {
        using base_class = dataflow_step<context_type, bool(void)>;
      public:
        handler(
          const context_type & context,
          const connection & args
        ) : base_class(context),
          owner_(args.owner_),
          tunnel_(true, args.owner_->client_certificate_, args.owner_->client_private_key_),
          done_count_(0),
          done_handler_(this),
          error_handler_(this)
        {
        }

        bool operator()(const service_provider & sp, network_socket & s)
        {
          this->owner_->owner_->on_connected(sp);
          
          vds::dataflow(
            input_network_stream(s),
            ssl_input_stream(this->tunnel_),
            http_response_parser(),
            input_command_stream(this->owner_, this->tunnel_)
          )
          (
            this->done_handler_,
            this->error_handler_,
            sp
          );
          
          vds::dataflow(
            output_command_stream(this->owner_),
            http_request_serializer(),
            ssl_output_stream(this->tunnel_),
            output_network_stream(s)
          )
          (
            this->done_handler_,
            this->error_handler_,
            sp);
          
          return false;
        }

      private:
        class stream_done
        {
        public:
          stream_done(handler * owner)
            : owner_(owner)
          {
          }

          void operator()(const service_provider & sp)
          {
            this->owner_->done_mutex_.lock();
            this->owner_->done_count_++;
            if (this->owner_->done_count_ == 2) {
              this->owner_->done_mutex_.unlock();
              this->owner_->next(sp);
            }
            else {
              this->owner_->done_mutex_.unlock();
            }
          }

        private:
          handler * owner_;
        };


        class stream_error
        {
        public:
          stream_error(handler * owner)
            : owner_(owner)
          {
          }

          void operator()(const service_provider & sp, std::exception_ptr ex)
          {
            sp.get<logger>()->error(
              sp,
              "stream %s:%d error: %s",
              this->owner_->owner_->address_.c_str(),
              this->owner_->owner_->port_,
              exception_what(ex));
            this->owner_->done_mutex_.lock();
            this->owner_->done_count_++;
            if (this->owner_->done_count_ == 2) {
              this->owner_->done_mutex_.unlock();
              this->owner_->error(sp, ex);
            }
            else {
              this->owner_->done_mutex_.unlock();
            }
          }

        private:
          handler * owner_;
        };
        
        _https_pipeline * owner_;
        ssl_tunnel tunnel_;

        std::mutex done_mutex_;
        int done_count_;

        stream_done done_handler_;
        stream_error error_handler_;
      };
    private:
      _https_pipeline * owner_;
    };
    
    class command_processor
    {
    public:
      command_processor(_https_pipeline * owner, ssl_tunnel & tunnel)
        : owner_(owner), tunnel_(tunnel)
      {
      }

      template <typename context_type>
      class handler : public dataflow_step<context_type, bool(void)>
      {
        using base_class = dataflow_step<context_type, bool(void)>;
      public:
        handler(
          const context_type & context,
          const command_processor & args
        ) : base_class(context),
          owner_(args.owner_),
          tunnel_(args.tunnel_)
        {
        }

        bool operator()(const service_provider & sp, json_value * response)
        {
          if (nullptr == response) {
            return this->next(sp);
          }
          else {
            return this->owner_->owner_->on_response(sp, response);
          }

          //auto cert = this->tunnel_.get_tunnel_certificate();
          //this->owner_->log_.trace("Certificate subject %s", cert.subject().c_str());
          //this->owner_->log_.trace("Certificate issuer %s", cert.issuer().c_str());
        }
      private:
        _https_pipeline * owner_;
        ssl_tunnel & tunnel_;
      };
    private:
      _https_pipeline * owner_;
      ssl_tunnel & tunnel_;
    };

    class null_command_processor
    {
    public:
      null_command_processor(_https_pipeline * owner, ssl_tunnel & tunnel)
        : owner_(owner), tunnel_(tunnel)
      {
      }

      template <typename context_type>
      class handler : public dataflow_step<context_type, bool(void)>
      {
        using base_class = dataflow_step<context_type, bool(void)>;
      public:
        handler(
          const context_type & context,
          const null_command_processor & args
        ) : base_class(context),
          owner_(args.owner_),
          tunnel_(args.tunnel_)
        {
        }

        bool operator()(const service_provider & sp, const void * data, size_t len)
        {
          if (0 == len) {
            this->next(sp);
          }
          
          return true;
        }
      private:
        _https_pipeline * owner_;
        ssl_tunnel & tunnel_;
      };
    private:
      _https_pipeline * owner_;
      ssl_tunnel & tunnel_;
    };


    class input_command_stream
    {
    public:
      input_command_stream(_https_pipeline * owner, ssl_tunnel & tunnel)
        : owner_(owner), tunnel_(tunnel)
      {
      }

      template <typename context_type>
      class handler : public dataflow_step<context_type, bool(void)>
      {
        using base_class = dataflow_step<context_type, bool(void)>;
      public:
        handler(
          const context_type & context,
          const input_command_stream & args
        ) : base_class(context),
          owner_(args.owner_),
          tunnel_(args.tunnel_)
        {
        }

        bool operator()(
          const service_provider & sp,
          http_response * response,
          http_incoming_stream * incoming_stream)
        {
          if(nullptr == response){
            return this->next(sp);
          }
          
          std::string content_type;
          response->get_header("Content-Type", content_type);

          if ("application/json" == content_type) {
            dataflow(
              http_stream_reader<typename base_class::prev_step_t>(this->prev, *incoming_stream),
              json_parser("client response"),
              command_processor(this->owner_, this->tunnel_)
            )
            (
              this->prev,
              this->error,
              sp);
          }
          else {
            dataflow(
              http_stream_reader<typename base_class::prev_step_t>(this->prev, *incoming_stream),
              null_command_processor(this->owner_, this->tunnel_)
            )
            (
              this->prev,
              this->error,
              sp);
          }
          
          return false;
        }
      private:
        _https_pipeline * owner_;
        ssl_tunnel & tunnel_;
      };
    private:
      _https_pipeline * owner_;
      ssl_tunnel & tunnel_;
    };

    class ioutput_command_stream
    {
    public:
      virtual void run(const service_provider & sp, const std::string & body) = 0;
    };
    class output_command_stream
    {
    public:
      output_command_stream(_https_pipeline * owner)
        : owner_(owner)
      {
      }

      template <typename context_type>
      class handler
        : public dataflow_step<context_type, bool(http_request * request, http_outgoing_stream * outgoing_stream)>,
          public ioutput_command_stream
      {
        using base_class = dataflow_step<context_type, bool(http_request * request, http_outgoing_stream * outgoing_stream)>;
      public:
        handler(
          const context_type & context,
          const output_command_stream & args
        ) : base_class(context),
          owner_(args.owner_),
          request_("POST", "/vds/client_api")
        {
          this->owner_->output_command_stream_ = this;
        }

        ~handler()
        {
        }

        bool operator()(const service_provider & sp)
        {
          this->processed(sp);
          return false;
        }

        void processed(const service_provider & sp)
        {
          this->owner_->owner_->get_commands(sp);
        }

        void run(const service_provider & sp, const std::string & body) override
        {
          sp.get<logger>()->trace(sp, "Send message %s", body.c_str());
          outgoing_stream_.set_body(body);
          this->next(sp, &this->request_, &this->outgoing_stream_);
        }

      private:
        _https_pipeline * owner_;

        http_request request_;
        http_outgoing_stream outgoing_stream_;
      };
    private:
      _https_pipeline * owner_;
    };
  };
}

#endif // __VDS_HTTP_HTTPS_PIPELINE_P_H_
