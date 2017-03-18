#ifndef __VDS_SERVER_CERT_MANAGER_H_
#define __VDS_SERVER_CERT_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class cert_manager
  {
  public:
    bool validate(const certificate & cert, event_handler<certificate *> & download_certificate_callback);
  };
}

#endif // __VDS_SERVER_CERT_MANAGER_H_
