#ifndef __VDS_P2P_NETWORK_NODE_ID_T_H_
#define __VDS_P2P_NETWORK_NODE_ID_T_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "vds_debug.h"
#include "guid.h"
#include "binary_serialize.h"

namespace vds {
  class node_id_t {
  public:
    static constexpr size_t SIZE = 16;

    node_id_t() {
      memset(this->id_, 0, sizeof(this->id_));
    }

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
        const auto b = (uint8_t)(this->id_[i] ^ data[i]);
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

    int operator != (const node_id_t & other) const {
      return memcmp(this->id_, other.id_, sizeof(this->id_)) != 0;
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

    node_id_t generate_random_id(uint8_t exp_index) const {
      uint8_t result[SIZE];
      memcpy(result, this->id_, sizeof(this->id_));

      result[exp_index / 8] ^= (0x80 >> (exp_index % 8));
      result[exp_index / 8] ^= ((0x80 >> (exp_index % 8)) - 1) & (uint8_t)std::rand();

      for(uint8_t i = exp_index / 8 + 1; i < SIZE; ++i){
        result[i] = (uint8_t)std::rand();
      }

      vds_assert(exp_index == distance_exp(result, SIZE));

      return node_id_t(result, SIZE);
    }

    bool operator !() const {
      for(size_t i = 0; i < SIZE; ++i){
        if(0 != this->id_[i]){
          return false;
        }
      }

      return true;
    }

    void clear() {
      memset(this->id_, 0, sizeof(this->id_));
    }

    std::string str() const {
      return this->device_id().str();
    }

  private:
    uint8_t id_[SIZE];
  };
}

inline vds::binary_serializer & operator << (vds::binary_serializer & s, const vds::node_id_t & value){
  s.push_data(value.id(), vds::node_id_t::SIZE, false);
  return s;
}

inline vds::binary_deserializer & operator >> (vds::binary_deserializer & s, vds::node_id_t & value){
  size_t size = vds::node_id_t::SIZE;
  s.pop_data(value.id(), size, false);
  return s;
}

#endif //__VDS_P2P_NETWORK_NODE_ID_T_H_
