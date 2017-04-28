#ifndef __VDS_CLIENT_CLIENT_CONNECTION_H_
#define __VDS_CLIENT_CLIENT_CONNECTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  template <typename connection_handler_type>
  class client_connection : public https_pipeline
  {
    using base_class = https_pipeline;
  public:
    client_connection(
      const service_provider & sp,
      connection_handler_type * handler,
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key
    )
      : base_class(sp, address, port, client_certificate, client_private_key),
      sp_(sp),
      log_(sp, "Client connection"),
      handler_(handler),
      state_(NONE)
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
      sp.get<logger>()(ll_debug, "Connecting to %s:%d", this->address().c_str(), this->port());

      this->state_ = CONNECTING;
      this->connection_start_ = std::chrono::system_clock::now();
      
      base_class::connect();
    }
   

    const std::chrono::time_point<std::chrono::system_clock> & connection_start() const
    {
      return this->connection_start_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_end() const
    {
      return this->connection_end_;
    }
    
    bool filter_messages(std::list<std::string> & messages, std::list<std::string> & target)
    {
      for(auto & m : messages){
        target.push_back(m);
      }
      
      messages.clear();
      return !target.empty();
    }

    void run(std::list<std::string> & messages)
    {
      std::ostringstream stream;
      stream << "[";
      bool first = true;
      for (auto & p : messages) {
        if (first) {
          first = false;
        }
        else {
          stream << ",";
        }

        stream << p;
      }
      stream << "]";

      base_class::run(stream.str());
    }

  protected:
    void on_connected() override
    {
      sp.get<logger>()(ll_debug, "Connected to %s:%d", this->address().c_str(), this->port());
      this->state_ = CONNECTED;
    }
    
    void on_connection_closed() override
    {
      sp.get<logger>()(ll_debug, "Connection to %s:%d has been closed", this->address().c_str(), this->port());
      this->connection_end_ = std::chrono::system_clock::now();
      this->state_ = NONE;
      this->handler_->connection_closed(*this);
    }

    void on_response(json_value * response) override
    {
      this->handler_->process_response(*this, response);
    }
    
    void on_error(std::exception_ptr ex) override
    {
      sp.get<logger>()(ll_debug, "Failed to connect %s:%d %s", this->address().c_str(), this->port(), exception_what(ex).c_str());
      this->connection_end_ = std::chrono::system_clock::now();
      this->state_ = CONNECT_ERROR;
      this->handler_->connection_error(*this, ex);
    }
    
    void get_commands()
    {
      this->handler_->get_commands(*this);
    }
    
  private:
    service_provider sp_;
    logger log_;
    connection_handler_type * handler_;
    connection_state state_;

    std::chrono::time_point<std::chrono::system_clock> connection_start_;
    std::chrono::time_point<std::chrono::system_clock> connection_end_;
  };
}


#endif // __VDS_CLIENT_CLIENT_CONNECTION_H_
