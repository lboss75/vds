#ifndef __VDS_STORAGE_CERTIFICATE_AUTHORITY_H_
#define __VDS_STORAGE_CERTIFICATE_AUTHORITY_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _certificate_authority;
  
  class certificate_authority
  {
  public:
    certificate_authority();
    ~certificate_authority();
    
    static certificate create_server(
      const certificate & user_certificate,
      const certificate & user_private_key,
      const asymmetric_private_key& server_private_key);

  private:
      _certificate_authority * const impl_;
  };
}

#endif // __VDS_STORAGE_CERTIFICATE_AUTHORITY_H_
