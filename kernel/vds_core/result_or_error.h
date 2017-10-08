#ifndef __VDS_CORE_RESULT_OR_ERROR_H_
#define __VDS_CORE_RESULT_OR_ERROR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  template <typename... result_types>
	class result_or_error
	{
  public:
    result_or_error()
    {
    }
    
    result_or_error(result_or_error<result_types...> && origin)
    : error_(std::move(origin.error_)), result_(std::move(origin.result_))
    {
    }
    
    const std::shared_ptr<std::exception> & error() const {
      return this->error_;
    }
    
    void error(const std::shared_ptr<std::exception> & new_value) {
      this->error_ = new_value;
    }
    
    void error(std::shared_ptr<std::exception> && new_value) {
      this->error_ = std::move(new_value);
    }
    
    const std::tuple<result_types...> & result() const {
      return this->result_;
    }
    
    void result(const std::tuple<result_types...> & new_value) const {
      this->result_ = new_value;
    }
    
    void result(std::tuple<result_types...> && new_value) const {
      this->result_ = std::move(new_value);
    }
    
  private:
    std::shared_ptr<std::exception> error_;
    std::tuple<result_types...> result_;
  };

}

#endif
