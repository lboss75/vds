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
    static constexpr size_t SIZE = 16;

    node_id_t(const uint8_t * data, size_t len) {
      vds_assert(len == sizeof(this->id_));
      memcpy(this->id_, data, sizeof(this->id_));
    }

    node_id_t(const guid & device_id) {
      vds_assert(device_id.size() == sizeof(this->id_));

      memcpy(this->id_, device_id.data(), sizeof(this->id_));
    }

    guid device_id() const {
      return guid(&this->id_, 16);
    }

    node_id_t distance(const uint8_t * data, size_t len) const {
      vds_assert(sizeof(this->id_) <= len);
      uint8_t result[sizeof(this->id_)];

      for(uint8_t i = 0; i < sizeof(this->id_); ++i)
      {
        result[i] = (this->id_[i] ^ data[i]);
      }

      return node_id_t(result, sizeof(result));
    }

    node_id_t distance(const node_id_t & other) const {
      return this->distance(other.id_, sizeof(other.id_));
    }

    node_id_t distance(const const_data_buffer & other) const {
      return this->distance(other.data(), other.size());
    }

    uint8_t distance_exp(const uint8_t * data, size_t len) const {
      vds_assert(sizeof(this->id_) <= len);
      for(uint8_t i = 0; i < sizeof(this->id_); ++i) {
        auto b = (this->id_[i] ^ data[i]);
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

    uint8_t distance_exp(const node_id_t & other) const {
      return this->distance_exp(other.id_, sizeof(other.id_));
    }

    uint8_t distance_exp(const const_data_buffer & other) const {
      return this->distance_exp(other.data(), other.size());
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

    const uint8_t * id() const {
      return this->id_;
    }

    uint8_t * id() {
      return this->id_;
    }

  private:
    uint8_t id_[SIZE];
  };
}

inline vds::binary_serializer & operator << (vds::binary_serializer & s, const node_id_t & value){
  s.push_data(value.id(), node_id_t::SIZE, false);
  return s;
}

inline vds::binary_deserializer & operator >> (vds::binary_deserializer & s, node_id_t & value){
  s.pop_data(value.id(), node_id_t::SIZE);
  return s;
}

#endif //__VDS_P2P_NETWORK_NODE_ID_T_H_
