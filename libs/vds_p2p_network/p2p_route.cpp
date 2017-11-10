#include "stdafx.h"
#include "p2p_route.h"
#include "private/p2p_route_p.h"

//////////////////////////////////////////////
vds::async_task<> vds::_p2p_route::send_to(const service_provider &sp, const guid &node_id,
                                           const const_data_buffer &message) {

  int best_distance;
  std::shared_ptr<session> best_session;

  for(;;) {
    std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);
    for (auto &p : this->sessions_) {
      int distance;
      if (!best_session || (best_distance > (distance = this->calc_distance(p.first, node_id)))) {
        p.second->lock();
        if (p.second->is_busy()) {
          p.second->unlock();
          continue;
        }

        best_distance = distance;
        if (best_session) {
          best_session->unlock();
        }

        best_session = p.second;
      }
    }

    if(best_session){
      break;
    }
  }

  return best_session->send(sp, node_id, message);
}

int vds::_p2p_route::calc_distance(const guid & source_node, const guid & target_node)
{
  return 0;
}

