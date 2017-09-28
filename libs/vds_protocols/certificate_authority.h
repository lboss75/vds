#ifndef __VDS_PROTOCOLS_CERTIFICATE_AUTHORITY_H_
#define __VDS_PROTOCOLS_CERTIFICATE_AUTHORITY_H_

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
      const guid & server_id,
      const certificate & user_certificate,
      const asymmetric_private_key & user_private_key,
      const asymmetric_private_key& server_private_key);

    static guid certificate_id(const certificate & cert);
    static guid certificate_parent_id(const certificate & cert);

    static certificate create_local_user(
      const guid & user_id,
      const certificate & owner_certificate,
      const asymmetric_private_key & owner_private_key,
      const asymmetric_private_key & user_private_key);

  private:
      _certificate_authority * const impl_;
  };
}

#endif // __VDS_PROTOCOLS_CERTIFICATE_AUTHORITY_H_
