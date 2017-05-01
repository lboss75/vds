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
      connection_handler_type * handler,
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key
    )
      : base_class(address, port, client_certificate, client_private_key),
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

    void connect(const service_provider & sp)
    {
      sp.get<logger>().debug(sp, "Connecting to %s:%d", this->address().c_str(), this->port());

      this->state_ = CONNECTING;
      this->connection_start_ = std::chrono::system_clock::now();
      
      base_class::connect(sp);
    }
   

    const std::chrono::time_point<std::chrono::system_clock> & connection_start() const
    {
      return this->connection_start_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_end() const
    {
      return this->connection_end_;
    }
    
    bool filter_messages(
      const service_provider & sp,
      std::list<std::string> & messages,
      std::list<std::string> & target)
    {
      for(auto & m : messages){
        target.push_back(m);
      }
      
      messages.clear();
      return !target.empty();
    }

    void run(
      const service_provider & sp,
      std::list<std::string> & messages)
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

      base_class::run(sp, stream.str());
    }

  protected:
    void on_connected(const service_provider & sp) override
    {
      sp.get<logger>().debug(sp, "Connected to %s:%d", this->address().c_str(), this->port());
      this->state_ = CONNECTED;
    }
    
    void on_connection_closed(const service_provider & sp) override
    {
      sp.get<logger>().debug(sp, "Connection to %s:%d has been closed", this->address().c_str(), this->port());
      this->connection_end_ = std::chrono::system_clock::now();
      this->state_ = NONE;
      this->handler_->connection_closed(sp, *this);
    }

    void on_response(const service_provider & sp, json_value * response) override
    {
      this->handler_->process_response(sp, *this, response);
    }
    
    void on_error(const service_provider & sp, std::exception_ptr ex) override
    {
      sp.get<logger>().debug(sp, "Failed to connect %s:%d %s", this->address().c_str(), this->port(), exception_what(ex).c_str());
      this->connection_end_ = std::chrono::system_clock::now();
      this->state_ = CONNECT_ERROR;
      this->handler_->connection_error(sp, *this, ex);
    }
    
    void get_commands(const service_provider & sp)
    {
      this->handler_->get_commands(sp, *this);
    }
    
  private:
    connection_handler_type * handler_;
    connection_state state_;

    std::chrono::time_point<std::chrono::system_clock> connection_start_;
    std::chrono::time_point<std::chrono::system_clock> connection_end_;
  };
}


#endif // __VDS_CLIENT_CLIENT_CONNECTION_H_
