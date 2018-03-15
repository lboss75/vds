#ifndef __VDS_P2P_NETWORK_NODE_ID_T_H_
#define __VDS_P2P_NETWORK_NODE_ID_T_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "vds_debug.h"
#include "guid.h"
#include "binary_serialize.h"
#include "resizable_data_buffer.h"

namespace vds {
  class dht_object_id {
  public:
    static const_data_buffer distance(const const_data_buffer & left, const const_data_buffer & right) {
      auto min_size = (left.size() < right.size()) ? left.size() : right.size();
      auto max_size = (left.size() > right.size()) ? left.size() : right.size();

      resizable_data_buffer result;
            
      for(size_t offset = 0; offset < min_size; ++offset) {
        result.add(left[offset] ^ right[offset]);
      }

      if(left.size() < right.size()) {
        result.add(right.data() + min_size, max_size - min_size);
      }
      else if (left.size() > right.size()) {
        result.add(left.data() + min_size, max_size - min_size);
      }

      return const_data_buffer(result.data(), result.size());
    }

    static size_t distance_exp(const const_data_buffer & left, const const_data_buffer & right) {
      auto min_size = (left.size() < right.size()) ? left.size() : right.size();

      for(size_t i = 0; i < min_size; ++i) {
        const auto b = (uint8_t)(left[i] ^ right[i]);
        if(0 == b){
          continue;
        }

        size_t result = (i << 3);

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

      if (left.size() == right.size()) {
        return 0;
      }
      else {
        return min_size << 3;
      }
    }


    static const_data_buffer generate_random_id(const const_data_buffer & original, uint8_t exp_index) {
      resizable_data_buffer result;
      result += original;

      result.data()[exp_index / 8] ^= (0x80 >> (exp_index % 8));
      result.data()[exp_index / 8] ^= ((0x80 >> (exp_index % 8)) - 1) & (uint8_t)std::rand();

      for(uint8_t i = exp_index / 8 + 1; i < original.size(); ++i){
        result.data()[i] = (uint8_t)std::rand();
      }

      vds_assert(exp_index == distance_exp(original, const_data_buffer(result.data(), result.size())));

      return const_data_buffer(result.data(), result.size());
    }
  };
}

#endif //__VDS_P2P_NETWORK_NODE_ID_T_H_
