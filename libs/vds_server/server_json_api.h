#ifndef __VDS_SERVER_SERVER_JSON_API_H_
#define __VDS_SERVER_SERVER_JSON_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_json_api
  {
  public:
    json_value * operator()(json_value * request) const
    {
      auto result = new json_object();
      result->add_property(new json_property("successful", new json_primitive("true")));
      return result;
    }
  };
}

#endif // __VDS_SERVER_SERVER_JSON_API_H_
