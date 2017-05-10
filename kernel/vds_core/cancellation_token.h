#ifndef __VDS_CORE_CANCELLATION_TOKEN_H_
#define __VDS_CORE_CANCELLATION_TOKEN_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <functional>

namespace vds {
  class cancellation_token_source;
  class _cancellation_token;
  
  class cancellation_subscriber
  {
  public:
    cancellation_subscriber();
    
    void destroy();
    
  private:
    int index_;
    std::shared_ptr<_cancellation_token> owner_;
    
    cancellation_subscriber(
      int index,
      const std::shared_ptr<_cancellation_token> & owner);
    
    friend class _cancellation_token;
  };
  
  class cancellation_token
  {
  public:
    cancellation_token(cancellation_token && );
    ~cancellation_token();
    
    bool is_cancellation_requested() const;
    
    cancellation_subscriber then_cancellation_requested(
      const std::function<void(void)> & callback);
    
  private:
    friend class cancellation_token_source;
    
    cancellation_token(const std::shared_ptr<_cancellation_token> & impl);
    std::shared_ptr<_cancellation_token> impl_;
  };
  
  class cancellation_token_source
  {
  public:
    cancellation_token_source();
    ~cancellation_token_source();
    
    cancellation_token token() const;
    
    void cancel();

  private:
    std::shared_ptr<_cancellation_token> impl_;
  };
}

#endif // __VDS_CORE_CANCELLATION_TOKEN_H_
