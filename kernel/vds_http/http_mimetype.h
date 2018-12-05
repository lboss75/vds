#ifndef __VDS_HTTP_HTTP_MIMETYPE_H_
#define __VDS_HTTP_HTTP_MIMETYPE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>


namespace vds {
  class filename;

  class http_mimetype
  {
  public:
    static std::string mimetype(const filename & fn);
  };
}

#endif // __VDS_HTTP_HTTP_MIMETYPE_H_
