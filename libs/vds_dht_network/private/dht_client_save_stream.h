#ifndef __VDS_DHT_NETWORK__DHT_CLIENT_SAVE_STREAM_H_
#define __VDS_DHT_NETWORK__DHT_CLIENT_SAVE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk.h"
#include "dht_network.h"

namespace vds {
  namespace dht {
    namespace network {
      class client_save_stream : public stream_output_async<uint8_t> {
      public:
        client_save_stream() = default;
        client_save_stream(client_save_stream &&) noexcept;
        client_save_stream(const client_save_stream &) = delete;

        static expected<client_save_stream> create(
          const service_provider* sp,
          std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> & generators);

        async_task<expected<void>> write_async(
          const uint8_t* data,
          size_t len) override;

        async_task<expected<std::vector<const_data_buffer>>> save();

        client_save_stream & operator = (client_save_stream &&) = default;

      private:
        struct generator_info_t {
          std::shared_ptr<hash_stream_output_async> hash_stream_;
          std::shared_ptr<chunk_output_async<uint16_t>> generator_;

          generator_info_t() = default;
          generator_info_t(const generator_info_t &) = delete;
          generator_info_t(generator_info_t &&) noexcept;

          static expected<generator_info_t> create(
            const service_provider* sp,
            chunk_generator<uint16_t>& generator);

          generator_info_t& operator =(const generator_info_t &) = delete;
          generator_info_t& operator =(generator_info_t&& origin) = default;

        private:
          generator_info_t(
            std::shared_ptr<hash_stream_output_async> && hash_stream,
            chunk_generator<uint16_t> & generator);
        };

        const service_provider * sp_;
        generator_info_t generators_[vds::dht::network::service::GENERATE_HORCRUX];

        client_save_stream(
          const service_provider* sp,
          generator_info_t (&&generators)[vds::dht::network::service::GENERATE_HORCRUX]);
      };
    }
  }
}



#endif//__VDS_DHT_NETWORK__DHT_CLIENT_SAVE_STREAM_H_
