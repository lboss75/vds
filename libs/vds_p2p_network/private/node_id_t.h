#ifndef __VDS_P2P_NETWORK_NODE_ID_T_H_
#define __VDS_P2P_NETWORK_NODE_ID_T_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "vds_debug.h"
#include "guid.h"

namespace vds {
  class node_id_t {
  public:
    node_id_t(const uint8_t * data, size_t len) {
      vds_assert(len == sizeof(this->id_));
      memcpy(this->id_, data, sizeof(this->id_));
    }

    node_id_t(
        const guid & device_id,
        const guid & session_id) {
      vds_assert(16 == device_id.size());
      vds_assert(16 == session_id.size());

      memcpy(this->id_, device_id.data(), 16);
      memcpy(this->id_ + 16, session_id.data(), 16);
    }

    guid device_id() const {
      return guid(&this->id_, 16);
    }

    guid session_id() const {
      return guid(&this->id_ + 16, 16);
    }

    node_id_t distance(const node_id_t & other) const {
      uint8_t result[sizeof(this->id_)];

      for(uint8_t i = 0; i < sizeof(this->id_); ++i)
      {
        result[i] = (this->id_[i] ^ other.id_[i]);
      }

      return node_id_t(result, sizeof(result));
    }

    uint8_t distance_exp(const node_id_t & other) const {
      for(uint8_t i = 0; i < sizeof(this->id_); ++i) {
        auto b = (this->id_[i] ^ other.id_[i]);
        if(0 == b){
          continue;
        }

        uint8_t result = (i << 3);

        uint8_t mask = 0x80;
        while(0 != mask){
          if(b >= mask){
            break;
          }

          ++result;
          mask >>= 1;
        }

        return result;
      }

      return 0;
    }

    int operator > (const node_id_t & other) const {
      return memcmp(this->id_, other.id_, sizeof(this->id_)) > 0;
    }

    int operator == (const node_id_t & other) const {
      return memcmp(this->id_, other.id_, sizeof(this->id_)) == 0;
    }

    int operator < (const node_id_t & other) const {
      return memcmp(this->id_, other.id_, sizeof(this->id_)) < 0;
    }

  private:
    uint8_t id_[32];//SHA256 (32 bytes) = Device Id (128 bit = 16 bytes) + Session Id (128 bit = 16 bytes)
  };

}

#endif //__VDS_P2P_NETWORK_NODE_ID_T_H_
