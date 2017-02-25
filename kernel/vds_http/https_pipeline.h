#ifndef __VDS_HTTP_HTTPS_PIPELINE_H_
#define __VDS_HTTP_HTTPS_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  class https_pipeline;
  class ihttps_pipeline_client
  {
  public:
    virtual void on_connected(https_pipeline & pipeline) = 0;
    virtual void on_response(https_pipeline & pipeline, json_value * response) = 0;
    virtual void on_error(https_pipeline & pipeline, std::exception * error) = 0;
  };
  
  class https_pipeline
  {
  public:
    https_pipeline(
      const service_provider & sp,
      ihttps_pipeline_client * client,
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key);
    ~https_pipeline();
    
    void connect();
    
    void push(json_value * request);

  private:
    const std::unique_ptr<class _https_pipeline> impl_;
  };
}

#endif // __VDS_HTTP_HTTPS_PIPELINE_H_
