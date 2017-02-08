#ifndef __VDS_PROTOCOLS_MESSAGE_H_
#define __VDS_PROTOCOLS_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  
  inline network_serializer & ask_certificate_and_key(network_serializer & sr, const std::string & object_name)
  {
    sr.start((uint16_t)1);
    sr << object_name;
    sr.final();

    return sr;
  };

}


#endif // __VDS_PROTOCOLS_MESSAGE_H_
