#ifndef __VDS_P2P_NETWORK_NODE_ID_T_H_
#define __VDS_P2P_NETWORK_NODE_ID_T_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "hash.h"
#include "vds_debug.h"
#include "guid.h"
#include "binary_serialize.h"
#include "resizable_data_buffer.h"
#include "encoding.h"
#include "crypto_service.h"

namespace vds {
  namespace dht {
    class dht_object_id {
    public:
      static const_data_buffer distance(const const_data_buffer &left, const const_data_buffer &right) {
        auto min_size = (left.size() < right.size()) ? left.size() : right.size();
        auto max_size = (left.size() > right.size()) ? left.size() : right.size();

        resizable_data_buffer result;

        for (size_t offset = 0; offset < min_size; ++offset) {
          result.add(left[offset] ^ right[offset]);
        }

        if (left.size() < right.size()) {
          result.add(right.data() + min_size, max_size - min_size);
        } else if (left.size() > right.size()) {
          result.add(left.data() + min_size, max_size - min_size);
        }

        return const_data_buffer(result.data(), result.size());
      }

      static size_t distance_exp(const const_data_buffer &left, const const_data_buffer &right) {
        auto min_size = (left.size() < right.size()) ? left.size() : right.size();

        for (size_t i = 0; i < min_size; ++i) {
          const auto b = (uint8_t) (left[i] ^ right[i]);
          if (0 == b) {
            continue;
          }

          size_t result = (i << 3);

          uint8_t mask = 0x80;
          while (0 != mask) {
            if (b >= mask) {
              break;
            }

            ++result;
            mask >>= 1;
          }

          return result;
        }

        if (left.size() == right.size()) {
          return 0;
        } else {
          return min_size << 3;
        }
      }

      static const_data_buffer generate_random_id() {
        uint8_t buffer[32];//MD5 size
        crypto_service::rand_bytes(buffer, sizeof(buffer));

        return const_data_buffer(buffer, sizeof(buffer));
      }

      static const_data_buffer generate_random_id(const const_data_buffer &original, size_t exp_index) {
        resizable_data_buffer result;
        result += original;

        result.data()[exp_index / 8] ^= (0x80 >> (exp_index % 8));
        result.data()[exp_index / 8] ^= ((0x80 >> (exp_index % 8)) - 1) & (uint8_t) std::rand();

        for (size_t i = exp_index / 8 + 1; i < original.size(); ++i) {
          result.data()[i] = (uint8_t) std::rand();
        }

        vds_assert(exp_index == distance_exp(original, const_data_buffer(result.data(), result.size())));

        return const_data_buffer(result.data(), result.size());
      }

      static const_data_buffer from_user_email(const std::string & user_email){
        return from_string("email:" + user_email);
      }

      static const_data_buffer my_record_channel(const std::string & user_email) {
        return from_string("my.records.channel:" + user_email);
      }

      static std::string user_credentials_to_key(const std::string & user_email, const std::string & user_password) {
        return user_credentials_to_key(
          user_email,
          hash::signature(hash::sha256(), user_password.c_str(), user_password.length()));
      }

      static std::string user_credentials_to_key(const std::string & user_email, const const_data_buffer & password_hash) {
        auto ph = base64::from_bytes(password_hash);
        return "credentials:"
          + std::to_string(user_email.length()) + "." + user_email + ","
          + std::to_string(ph.length()) + "." + ph;
      }

    private:
      static const_data_buffer from_string(const std::string & value){
        return hash::signature(hash::sha256(), value.c_str(), value.length());
      }
    };
  }
}

#endif //__VDS_P2P_NETWORK_NODE_ID_T_H_
