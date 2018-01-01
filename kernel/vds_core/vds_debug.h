#ifndef __VDS_CORE_VDS_DEBUG_H_
#define __VDS_CORE_VDS_DEBUG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

#define vds_assert(exp) if(!(exp)) { throw std::runtime_error("Accert " #exp); }

}

#endif //__VDS_CORE_VDS_DEBUG_H_
