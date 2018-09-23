#ifndef __VDS_CORE_ASYNC_LOOP_H_
#define __VDS_CORE_ASYNC_LOOP_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {

  template <typename value_type>
  class async_loop_token {
  public:
    async_loop_token(
        const vds::async_result<value_type> & result,
        const std::function<void (const async_loop_token<value_type> & token)> & callback)
    : result_(result), callback_(callback){
    }

    void continue_loop() const {
      this->callback_(*this);
    }

    void break_loop(const value_type & result) const {
      this->result_.done(result);

    }

    void error(const std::shared_ptr<std::exception>& ex) const {
      this->result_.error(ex);
    }

  private:
    vds::async_result<value_type> result_;
    std::function<void (const async_loop_token<value_type> & token)> callback_;
  };

  template <typename value_type>
  inline vds::async_task<value_type> async_loop(const std::function<void (const async_loop_token<value_type> & token)> & callback){
    return [callback](const vds::async_result<value_type> & result){
      async_loop_token token(result, callback);
      token.continue_loop();
    };
  }

}
#endif //__VDS_CORE_ASYNC_LOOP_H_
