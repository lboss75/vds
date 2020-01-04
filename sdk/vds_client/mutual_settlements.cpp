/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "mutual_settlements.h"
//#include "transaction_block.h"
//#include "store_block_transaction.h"
//#include "dht_network_client.h"
//#include "user_manager.h"
//#include "db_model.h"
//#include "user_wallet.h"
//
//vds::mutual_settlements::mutual_settlements()
//  : last_calculate_(std::chrono::steady_clock::now())
//{
//}
//
//vds::expected<void> vds::mutual_settlements::update(
//  const transactions::transaction_block & block,
//  const transactions::store_block_transaction & message)
//{
//  this->blocks_[message.object_id] = payment_info{ message.object_size, block.time_point(), block.time_point() };
//  return expected<void>();
//}
//
//vds::expected<void> vds::mutual_settlements::update(const transactions::transaction_block & block, const transactions::payment_transaction & message)
//{
//  GET_EXPECTED(object_id, base64::to_bytes(message.notes));
//  this->blocks_[object_id].last_payment_time_ = block.time_point();
//  return expected<void>();
//}
//
//vds::expected<void> vds::mutual_settlements::update(const transactions::transaction_block & block, const transactions::payment_request_transaction & message)
//{
//  return expected<void>();
//}
//
//vds::expected<void> vds::mutual_settlements::calculate(
//  const service_provider * sp,
//  const std::list<std::shared_ptr<vds::user_wallet>>& wallets,
//  database_read_transaction & t)
//{
//  if (nullptr == this) {
//    return expected<void>();
//  }
//
//  if (this->last_calculate_ < std::chrono::steady_clock::now() - std::chrono::hours(1)) {
//    return expected<void>();
//  }
//
//  GET_EXPECTED(block, transactions::transaction_block_builder::create(sp, t));
//
//  this->last_calculate_ = std::chrono::steady_clock::now();
//  for (const auto & p : this->blocks_) {
//    if (p.second.last_payment_time_ < std::chrono::system_clock::now() - std::chrono::hours(10 * 24)) {
//      auto rp = p.second.payment_requests_.cbegin();
//      
//      auto last_request_time = rp->second.created_time_;
//      auto last_request = rp->second.transaction_;
//      ++rp;
//      while(rp != p.second.payment_requests_.cend()){
//        if (last_request_time < rp->second.created_time_) {
//          last_request_time = rp->second.created_time_;
//          last_request = rp->second.transaction_;
//        }
//        ++rp;
//      }
//      if (last_request.value < (last_request_time - p.second.last_payment_time_).count() * p.second.block_size / 1024 / 1024) {
//        auto value = last_request.value;
//        for (auto & wallet : wallets) {
//          GET_EXPECTED(v, wallet->transfer(block, t, last_request.issuer, last_request.currency, last_request.target_wallet, value, last_request.payment_type, last_request.notes));
//          value -= v;
//          if (value == 0) {
//            break;
//          }
//        }
//        if(0 == value) {
//          CHECK_EXPECTED(sp->get<db_model>()->async_transaction([sp, b = std::move(block)](
//            database_transaction & t) mutable -> expected<void> {
//            CHECK_EXPECTED(sp->get<dht::network::client>()->save(
//              sp,
//              b,
//              t));
//            return expected<void>();
//          }).get());
//        }
//      }
//    }
//  }
//   
//  return expected<void>();
//
//}
