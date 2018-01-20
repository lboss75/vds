//
// Created by vadim on 30.10.17.
//

#ifndef __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_H_
#define __VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "async_task.h"

namespace vds {

  class p2p_network_service {
  public:
	  p2p_network_service();
	  ~p2p_network_service();

    vds::async_task<> start(
        const vds::service_provider &sp,
        int port,
        const std::list<class certificate> &certificate_chain,
        const class asymmetric_private_key &node_key);

		void stop(const service_provider & sp);
    async_task<> prepare_to_stop(const vds::service_provider &sp);

		class _p2p_network_service *operator ->() const { return  this->impl_.get();}
		operator bool () const {
			return nullptr != this->impl_.get();
		}
  private:
    std::shared_ptr<class _p2p_network_service> impl_;
  };

}


#endif //__VDS_P2P_NETWORK_P2P_NETWORK_SERVICE_H_
