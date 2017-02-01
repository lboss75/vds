#ifndef __VDS_BACKGROUND_VDS_BACKGROUND_APP_H_
#define __VDS_BACKGROUND_VDS_BACKGROUND_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class vrt_object;

  class vds_background_app : public vds_console_app<vds_background_app>
  {
    typedef vds_console_app<vds_background_app> base;
  public:
    vds_background_app();

    void main(const service_provider & sp);
    
    void register_services(service_registrator & registrator);
    
  private:
    network_service network_service_;
    http_router router_;
    http_server http_service_;
  };
}

#endif // __VDS_BACKGROUND_VDS_BACKGROUND_APP_H_
