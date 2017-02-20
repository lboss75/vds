#ifndef __VDS_CLIENT_CLIENT_CONNECTION_H_
#define __VDS_CLIENT_CLIENT_CONNECTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  template <typename connection_handler_type>
  class client_connection
  {
  public:
    client_connection(
      const service_provider & sp,
      connection_handler_type * handler,
      const std::string & address,
      int port,
      certificate & client_certificate,
      asymmetric_private_key & client_private_key
    )
      : 
      sp_(sp),
      log_(sp, "Client connection"),
      handler_(handler),
      address_(address),
      port_(port),
      client_certificate_(client_certificate),
      client_private_key_(client_private_key),
      state_(NONE),
      connect_done_(std::bind(&client_connection::connect_done, this)),
      connect_error_(std::bind(&client_connection::connect_error, this, std::placeholders::_1))
    {
    }

    ~client_connection()
    {

    }

    enum connection_state
    {
      NONE,
      CONNECTING,
      CONNECTED,
      CONNECT_ERROR
    };

    connection_state state() const
    {
      return this->state_;
    }

    void connect()
    {
      this->state_ = CONNECTING;
      this->connection_start_ = std::chrono::system_clock::now();

      auto sp = this->sp_.create_scope();
      vds::sequence(
        socket_connect(sp),
        connection(sp, this)
      )
      (
        this->connect_done_,
        this->connect_error_,
        this->address_,
        this->port_
      );
    }
    const std::string & address() const {
      return this->address_;
    }

    int port() const {
      return this->port_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_start() const
    {
      return this->connection_start_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_end() const
    {
      return this->connection_end_;
    }
    
    itask_manager get_task_manager() const;

  private:
    service_provider sp_;
    logger log_;
    connection_handler_type * handler_;
    std::string address_;
    int port_;
    certificate & client_certificate_;
    asymmetric_private_key & client_private_key_;

    connection_state state_;

    std::chrono::time_point<std::chrono::system_clock> connection_start_;
    std::chrono::time_point<std::chrono::system_clock> connection_end_;

    std::function<void(void)> connect_done_;
    std::function<void(std::exception *)> connect_error_;

    void connect_done(void)
    {
      this->log_(ll_debug, "Connected to %s:%d", this->address_.c_str(), this->port_);
      this->connection_end_ = std::chrono::system_clock::now();
      this->state_ = NONE;
      this->handler_->connection_closed(this);
    }

    void connect_error(std::exception * ex)
    {
      this->log_(ll_debug, "Failed to connect %s:%d %s", this->address_.c_str(), this->port_, ex->what());
      this->connection_end_ = std::chrono::system_clock::now();
      this->state_ = CONNECT_ERROR;
      this->handler_->connection_error(this, ex);
    }

    void on_connected(network_socket & s)
    {
      this->state_ = CONNECTED;
    }

    void process_response(json_value * response)
    {
      this->handler_->process_response(this, response);
    }

    class connection
    {
    public:
      connection(
        const service_provider & sp,
        client_connection * owner)
        : sp_(sp), owner_(owner)
      {
      }

      template <typename context_type>
      class handler : public sequence_step<context_type, void(void)>
      {
        using base_class = sequence_step<context_type, void(void)>;
      public:
        handler(
          const context_type & context,
          const connection & args
        ) : base_class(context),
          sp_(args.sp_),
          owner_(args.owner_),
          tunnel_(args.sp_, true, &args.owner_->client_certificate_, &args.owner_->client_private_key_),
          done_count_(0),
          done_handler_(this),
          error_handler_(this)
        {
        }

        void operator()(network_socket & s)
        {
          this->owner_->on_connected(s);
          
          vds::sequence(
            input_network_stream(this->sp_, s),
            ssl_input_stream(this->tunnel_),
            http_response_parser(),
            input_command_stream(this->owner_, this->tunnel_)
          )
          (
            this->done_handler_,
            this->error_handler_
          );
          
          vds::sequence(
            output_command_stream(this->owner_),
            http_request_serializer(),
            ssl_output_stream(this->tunnel_),
            output_network_stream(this->sp_, s)
          )
          (
            this->done_handler_,
            this->error_handler_
          );
        }

      private:
        class stream_done
        {
        public:
          stream_done(handler * owner)
            : owner_(owner)
          {
          }

          void operator()()
          {
            this->owner_->done_mutex_.lock();
            this->owner_->done_count_++;
            if (this->owner_->done_count_ == 2) {
              this->owner_->done_mutex_.unlock();
              this->owner_->next();
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

          void operator()(std::exception * ex)
          {
            this->owner_->owner_->log_(ll_error, "stream %s:%d error: %s", this->owner_->owner_->address_.c_str(), this->owner_->owner_->port_, ex->what());
            this->owner_->done_mutex_.lock();
            this->owner_->done_count_++;
            if (this->owner_->done_count_ == 2) {
              this->owner_->done_mutex_.unlock();
              this->owner_->error(ex);
            }
            else {
              this->owner_->done_mutex_.unlock();
            }
          }

        private:
          handler * owner_;
        };
        
        service_provider sp_;
        client_connection * owner_;
        ssl_tunnel tunnel_;

        std::mutex done_mutex_;
        int done_count_;

        stream_done done_handler_;
        stream_error error_handler_;
      };
    private:
      service_provider sp_;
      client_connection * owner_;
    };
    
    class command_processor
    {
    public:
      command_processor(client_connection * owner, ssl_tunnel & tunnel)
        : owner_(owner), tunnel_(tunnel)
      {
      }

      template <typename context_type>
      class handler : public sequence_step<context_type, void(void)>
      {
        using base_class = sequence_step<context_type, void(void)>;
      public:
        handler(
          const context_type & context,
          const command_processor & args
        ) : base_class(context),
          owner_(args.owner_),
          tunnel_(args.tunnel_)
        {
        }

        void operator()(json_value * response)
        {
          if (nullptr == response) {
            this->next();
          }
          else {
            this->owner_->process_response(response);
          }

          //auto cert = this->tunnel_.get_tunnel_certificate();
          //this->owner_->log_.trace("Certificate subject %s", cert.subject().c_str());
          //this->owner_->log_.trace("Certificate issuer %s", cert.issuer().c_str());
        }
      private:
        client_connection * owner_;
        ssl_tunnel & tunnel_;
      };
    private:
      client_connection * owner_;
      ssl_tunnel & tunnel_;
    };

    class null_command_processor
    {
    public:
      null_command_processor(client_connection * owner, ssl_tunnel & tunnel)
        : owner_(owner), tunnel_(tunnel)
      {
      }

      template <typename context_type>
      class handler : public sequence_step<context_type, void(void)>
      {
        using base_class = sequence_step<context_type, void(void)>;
      public:
        handler(
          const context_type & context,
          const null_command_processor & args
        ) : base_class(context),
          owner_(args.owner_),
          tunnel_(args.tunnel_)
        {
        }

        void operator()(const void * data, size_t len)
        {
          if (0 == len) {
            this->next();
          }
        }
      private:
        client_connection * owner_;
        ssl_tunnel & tunnel_;
      };
    private:
      client_connection * owner_;
      ssl_tunnel & tunnel_;
    };


    class input_command_stream
    {
    public:
      input_command_stream(client_connection * owner, ssl_tunnel & tunnel)
        : owner_(owner), tunnel_(tunnel)
      {
      }

      template <typename context_type>
      class handler : public sequence_step<context_type, void(void)>
      {
        using base_class = sequence_step<context_type, void(void)>;
      public:
        handler(
          const context_type & context,
          const input_command_stream & args
        ) : base_class(context),
          owner_(args.owner_),
          tunnel_(args.tunnel_)
        {
        }

        void operator()(
          http_response & response,
          http_incoming_stream & incoming_stream)
        {
          std::string content_type;
          response.get_header("Content-Type", content_type);

          if ("application/json" == content_type) {
            sequence(
              http_stream_reader<typename base_class::prev_step_t>(this->prev, incoming_stream),
              json_parser("ping response"),
              command_processor(this->owner_, this->tunnel_)
            )
            (
              this->prev,
              this->error
              );
          }
          else {
            sequence(
              http_stream_reader<typename base_class::prev_step_t>(this->prev, incoming_stream),
              null_command_processor(this->owner_, this->tunnel_)
            )
            (
              this->prev,
              this->error
            );
          }
        }
      private:
        client_connection * owner_;
        ssl_tunnel & tunnel_;
      };
    private:
      client_connection * owner_;
      ssl_tunnel & tunnel_;
    };

    class output_command_stream
    {
    public:
      output_command_stream(client_connection * owner)
        : owner_(owner)
      {
      }

      template <typename context_type>
      class handler : public sequence_step<context_type, void(http_request & request, http_outgoing_stream & outgoing_stream)>
      {
        using base_class = sequence_step<context_type, void(http_request & request, http_outgoing_stream & outgoing_stream)>;
      public:
        handler(
          const context_type & context,
          const output_command_stream & args
        ) : base_class(context),
          owner_(args.owner_),
          timer_job_(std::bind(&handler::timer_job, this)),
          ping_job_(itask_manager::get(this->owner_->sp_).create_job("server ping", this->timer_job_)),
          request_("POST", "/vds/ping")
        {
        }

        ~handler()
        {
          this->ping_job_.destroy();
        }

        void operator()()
        {
          this->ping_job_.schedule(
            std::chrono::system_clock::now()
            + std::chrono::seconds(1));
          //this->processed();
        }

        void processed()
        {
          this->ping_job_.schedule(
            std::chrono::system_clock::now()
            + std::chrono::seconds(1));
        }

      private:
        std::chrono::time_point<std::chrono::system_clock> next_message_;
        client_connection * owner_;

        std::function<void(void)> timer_job_;
        task_job ping_job_;

        http_request request_;
        http_outgoing_stream outgoing_stream_;

        void timer_job()
        {
          try {
            this->outgoing_stream_.set_body(this->owner_->handler_->get_messages());
            this->next(this->request_, this->outgoing_stream_);
          }
          catch (std::exception * ex) {
            this->error(ex);
          }
        }
      };
    private:
      client_connection * owner_;
    };
  };
}


#endif // __VDS_CLIENT_CLIENT_CONNECTION_H_
