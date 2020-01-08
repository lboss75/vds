/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_api.h"
#include "db_model.h"
#include "dht_network_client.h"
#include "../private/dht_network_client_p.h"

vds::async_task<vds::expected<vds::server_api::data_info_t>> vds::server_api::upload_data(
  const const_data_buffer& body)
{
  vds::server_api::data_info_t result;
  GET_EXPECTED_VALUE_ASYNC(result.data_hash, hash::signature(hash::sha256(), body));
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction(
    [
      sp = this->sp_,
      body,
      &result
    ](database_transaction& t)->expected<void> {
      auto network_client = sp->get<dht::network::client>();
      GET_EXPECTED_VALUE(result.replicas, (*network_client)->save_temp(t, result.data_hash, body, &result.replica_size));

      return expected<void>();
    }));

  co_return result;
}

vds::async_task<vds::expected<vds::const_data_buffer>> vds::server_api::broadcast(const const_data_buffer& body)
{
  const_data_buffer trx_id;
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction(
    [
      sp = this->sp_,
      body,
      &trx_id
    ](database_transaction& t)->expected<void> {
      transactions::transaction_block_builder playback;

      CHECK_EXPECTED(playback.push_data(body));

      auto network_client = sp->get<dht::network::client>();
      GET_EXPECTED_VALUE(trx_id, network_client->save(sp, playback, t));

      return expected<void>();
    }));

  co_return trx_id;
}
