#ifndef __VDS_HTTP_HTTPS_PIPELINE_H_
#define __VDS_HTTP_HTTPS_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  class https_pipeline
  {
  public:
    https_pipeline(
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key);
    ~https_pipeline();
    
    void connect(const service_provider & sp);
   
    const std::string & address() const;
    int port() const;
   
  protected:
    friend class _https_pipeline;
    
    virtual void on_connected(const service_provider & sp);
    virtual void on_connection_closed(const service_provider & sp);
    virtual void on_error(const service_provider & sp, const std::shared_ptr<std::exception> & error);
    
    virtual bool on_response(const service_provider & sp, json_value * response) = 0;
    virtual void get_commands(const service_provider & sp) = 0;

    void run(const service_provider & sp, const std::string & body);
    
  private:
    const std::unique_ptr<class _https_pipeline> impl_;
  };
}

#endif // __VDS_HTTP_HTTPS_PIPELINE_H_
