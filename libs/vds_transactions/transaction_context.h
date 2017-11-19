#ifndef __VDS_TRANSACTIONS__TRANSACTION_CONTEXT_H_
#define __VDS_TRANSACTIONS__TRANSACTION_CONTEXT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "guid.h"

namespace vds {

  class transaction_context : public service_provider::property_holder {
  public:
    transaction_context(const guid & author_id)
    : author_id_(author_id){
    }

    const guid & author_id() const { return  this->author_id_; }

  private:
    guid author_id_;
  };
}

#endif //__VDS_TRANSACTIONS__TRANSACTION_CONTEXT_H_
