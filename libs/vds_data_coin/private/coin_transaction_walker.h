#ifndef __VDS_DATA_COIN_COIN_TRANSACTION_WALKER_H_
#define __VDS_DATA_COIN_COIN_TRANSACTION_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "database.h"
#include "coin_transaction_package.h"

namespace vds {
  namespace data_coin_private {
    class coin_transaction_walker {
    public:

      static coin_transaction_walker start_with(
          const const_data_buffer & package_id,
          uint64_t  order_no) {
        return coin_transaction_walker(package_id, order_no);
      }

      bool is_processed(
          database_transaction & t,
          const const_data_buffer & package_id,
          uint64_t  order_no){

        while(this->max_processed_ > order_no){
          auto porder_no = this->process_state_.find(this->max_processed_);
          if(this->process_state_.end() != porder_no) {
            for (auto p : porder_no->second) {
              if (p.second) {
                for (auto base_package : data_coin::coin_transaction_package::get_base_packages(t, p.first)) {
                  auto order_no = data_coin::coin_transaction_package::get_order_no(t, base_package);
                  if (order_no >= this->max_processed_) {
                    throw std::runtime_error("Database is corrupted");
                  }

                  this->process_state_[order_no][base_package] = true;
                }
              }
            }
            this->max_processed_--;
          }
        }

        auto porder_no = this->process_state_.find(order_no);
        if(this->process_state_.end() != porder_no){
          auto ppackage_id = porder_no->second.find(package_id);
          if(porder_no->second.end() != ppackage_id) {
            return ppackage_id->second;
          }
        }

        return false;
      }

      void schedule_package(
          database_transaction & t,
          const const_data_buffer & package_id,
          uint64_t  order_no){
        if(this->is_processed(t, package_id, order_no)){
          return;
        }

        this->process_state_[order_no][package_id] = false;
        if(this->max_unprocessed_ < order_no){
          this->max_unprocessed_ = order_no;
        }
      }

      bool next_unprocessed(
          const_data_buffer & package_id,
          uint64_t  & order_no) {
        while(this->max_unprocessed_ > 0){
          auto p = this->process_state_.find(this->max_unprocessed_);
          if(this->process_state_.end() == p){
            this->max_unprocessed_--;
            continue;
          }

          for(auto pp : p->second){
            if(!pp.second){
              package_id = pp.first;
              order_no = this->max_unprocessed_;
              pp.second = true;
              return true;
            }
          }
        }

        return false;
      }

    private:
      std::map<uint64_t, std::map<const_data_buffer, bool>> process_state_;
      uint64_t max_processed_;
      uint64_t max_unprocessed_;

      coin_transaction_walker(coin_transaction_walker && original)
      : process_state_(std::move(original.process_state_)),
        max_processed_(original.max_processed_),
        max_unprocessed_(original.max_unprocessed_){
      }

      coin_transaction_walker(
          const const_data_buffer & package_id,
          uint64_t  order_no)
      : max_processed_(order_no),
        max_unprocessed_(0){

        this->process_state_[order_no][package_id] = true;
      }
    };
  }
}

#endif // __VDS_DATA_COIN_COIN_TRANSACTION_WALKER_H_

