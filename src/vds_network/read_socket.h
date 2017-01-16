/* inclusion guard */
#ifndef __VDS_NETWORK_READ_SOCKET_H__
#define __VDS_NETWORK_READ_SOCKET_H__

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class read_socket
  {
  public:
    read_socket(
      const network_socket & s,
      size_t buffer_size
    );
    
    class context
    {
    public:
      
      template <typename next_filter>
      void operator()(
        const error_handler_t & on_error,
        next_filter next) {
        
        this->network_service_
        .read_async<next_filter>(
          next,
          on_error,
          this->s_,
          this->buffer_.data(),
          this->buffer_.size()
        );
      }
      
    private:
      network_service & network_service_;
      network_socket * s_;
      std::vector buffer_;
    };
    
    
  };
}

#endif /* __VDS_NETWORK_READ_SOCKET_H__ */
