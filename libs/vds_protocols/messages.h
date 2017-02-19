#ifndef __VDS_PROTOCOLS_MESSAGE_H_
#define __VDS_PROTOCOLS_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  
  class messages
  {
  public:
    class ask_certificate_and_key
    {
    public:
      ask_certificate_and_key(
        const std::string & object_name
      );

    private:
      std::string object_name_;
    };
    
    
  };
}


#endif // __VDS_PROTOCOLS_MESSAGE_H_
