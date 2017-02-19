#ifndef __VDS_HTTP_HTTP_JSON_API_H_
#define __VDS_HTTP_HTTP_JSON_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
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
    class handler : public sequence_step<context_type, void (json_value * response)>
    {
      using base_class = sequence_step<context_type, void (json_value * response)>;
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
      
      void operator()(json_value * request)
      {
        try
        {
          this->response_.reset(this->handler_(this->scope_, request));
        }
        catch(std::exception * ex)
        {
          auto response = new json_object();
          response->add_property(new json_property("$type", new json_primitive(typeid(ex).name())));
          response->add_property(new json_property("$result", new json_primitive("exception")));
          response->add_property(new json_property("message", new json_primitive(ex->what())));
          this->response_.reset(response);
        }
        
        this->next(this->response_.get());
      }
      
    private:
      service_provider scope_;
      const handler_type & handler_;
      std::unique_ptr<json_value> response_;
    };

  private:
    service_provider scope_;
    const handler_type & handler_;
  };
};

#endif // __VDS_HTTP_HTTP_JSON_API_H_
