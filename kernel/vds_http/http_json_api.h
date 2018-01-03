#ifndef __VDS_HTTP_HTTP_JSON_API_H_
#define __VDS_HTTP_HTTP_JSON_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

/*
namespace vds {
  template <typename handler_type>
  class http_json_api
  {
  public:
    http_json_api(
      const service_provider & scope,
      const handler_type & handler
    )
    : scope_(scope), handler_(handler)
    {
    }
    
    template <typename context_type>
    class handler : public dataflow_step<context_type, bool (json_value * response)>
    {
      using base_class = dataflow_step<context_type, bool (json_value * response)>;
    public:
      handler(
        const context_type & context,
        const http_json_api & args
      )
      : base_class(context),
        scope_(args.scope_),
        handler_(args.handler_)
      {        
      }
      
      bool operator()(const service_provider & sp, json_value * request)
      {
        try
        {
          this->response_.reset(this->handler_(this->scope_, request));
        }
        catch(...)
        {
          auto response = new json_object();
          response->add_property("$t", typeid(std::current_exception()).name());
          response->add_property("r", "exception");
          response->add_property("m", exception_what(std::current_exception()));
          this->response_.reset(response);
        }
        
        return this->next(sp, this->response_.get());
      }
      
    private
      service_provider scope_;
      const handler_type & handler_;
      std::shared_ptr<json_value> response_;
    };

  private
    service_provider scope_;
    const handler_type & handler_;
  };
};
*/

#endif // __VDS_HTTP_HTTP_JSON_API_H_
