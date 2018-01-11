#ifndef __VDS_CORE_VDS_EXCEPTIONS_H_
#define __VDS_CORE_VDS_EXCEPTIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <stdexcept>

namespace vds {
  namespace vds_exceptions {
    class not_found : public std::runtime_error {
    public:
      not_found();

    };

    class invalid_operation : public std::runtime_error {
    public:
      invalid_operation();

    };

	class signature_validate_error : public std::runtime_error {
	public:
		signature_validate_error();

	};
  };
}


#endif //__VDS_CORE_VDS_EXCEPTIONS_H_
