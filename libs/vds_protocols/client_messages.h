#ifndef __VDS_PROTOCOLS_CLIENT_MESSAGES_H_
#define __VDS_PROTOCOLS_CLIENT_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class client_messages
  {
  public:
    class ask_certificate_and_key
    {
    public:
      static const char message_type[];
      
      ask_certificate_and_key(
        const std::string & request_id,
        const std::string & object_name        
      );
      
      ask_certificate_and_key(const json_value *);
      
      std::unique_ptr<json_object> serialize() const;

    private:
      std::string request_id_;
      std::string object_name_;
    };
  };
}

#endif // __VDS_PROTOCOLS_CLIENT_MESSAGES_H_
