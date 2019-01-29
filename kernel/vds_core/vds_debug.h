#ifndef __VDS_CORE_VDS_DEBUG_H_
#define __VDS_CORE_VDS_DEBUG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

#define vds_assert(exp) if(!(exp)) { throw std::runtime_error("Accert " #exp); }

  class thread_protect {
  public:
    thread_protect();
    ~thread_protect();

    static void check();
  };

  class thread_unprotect {
  public:
    thread_unprotect();
    ~thread_unprotect();

  private:
    bool original_;
  };

}

#endif //__VDS_CORE_VDS_DEBUG_H_
