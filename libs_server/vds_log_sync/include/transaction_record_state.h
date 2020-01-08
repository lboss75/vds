//#ifndef __VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
//#define __VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include <map>
//#include "payment_transaction.h"
//#include "binary_serialize.h"
//#include "database.h"
//#include "transaction_log_vote_request_dbo.h"
//
//
//namespace vds {
//  namespace transactions {
//    class transaction_block;
//
//    class transaction_record_state {
//    public:
//
//      transaction_record_state() {        
//      }
//
//      transaction_record_state(
//          const const_data_buffer & root_block_id,
//          const std::string & root_account) {
//        this->account_state_[root_account].balance_[root_block_id] = UINT64_MAX;
//      }
//
//      transaction_record_state(const transaction_record_state & original)
//        : account_state_(original.account_state_) {
//      }
//
//      transaction_record_state(transaction_record_state && original)
//      : account_state_(std::move(original.account_state_)){
//      }
//
//      static expected<transaction_record_state> load(
//        database_read_transaction & t,
//        const const_data_buffer & log_id);
//
//      static expected<transaction_record_state> load(
//        database_read_transaction & t,
//        const std::set<vds::const_data_buffer> & ancestors);
//
//      static expected<transaction_record_state> load(
//        database_read_transaction & t,
//        const transaction_block & block);
//      
//      expected<void> save(
//        database_transaction & t,
//        const std::string & owner,
//        const const_data_buffer & log_id) const;
//
//      struct account_state_t {
//        bool approve_required;
//        std::map<const_data_buffer/*source*/, int64_t /*balance*/> balance_;
//
//        account_state_t()
//        : approve_required(false) {          
//        }
//      };
//
//      expected<void> apply(
//        const transaction_block & block);
//
//      expected<void> rollback(
//        const transaction_block & block);
//
//
//      transaction_record_state &operator = (transaction_record_state && original){
//        this->account_state_ = std::move(original.account_state_);
//        return *this;
//      }
//      
//      const std::map<std::string/*account*/, account_state_t> & account_state() const {
//        return this->account_state_;
//      }
//
//    private:
//      friend class data_coin_balance;
//
//      std::map<std::string/*account*/, account_state_t> account_state_;
//
//      class transaction_state_calculator {
//      public:
//        transaction_state_calculator();
//
//        expected<void> add_ancestor(
//          vds::database_read_transaction &t,
//          const const_data_buffer & ancestor_id);
//
//        expected<transaction_record_state> load(
//          vds::database_read_transaction& t);
//
//      private:
//        enum class log_state_t {
//          none,
//
//          include,
//          exclude,
//
//          included,
//        };
//
//        struct log_state {
//          log_state_t state_;
//          const_data_buffer block_body_;
//        };
//
//        int not_included_;
//        std::map<uint64_t/*order_no*/, std::map<const_data_buffer/*log_id*/, log_state /*state*/>> items_;
//
//        void set_state(
//          uint64_t order_no,
//          const vds::const_data_buffer &log_id,
//          log_state_t state);
//        log_state_t get_state(
//          uint64_t order_no,
//          const vds::const_data_buffer &log_id) const;
//
//        expected<transaction_record_state> load_init_state(
//          vds::database_read_transaction& t);
//
//      };
//    };
//  }
//}
//
//#endif //__VDS_TRANSACTIONS_TRANSACTION_RECORD_STATE_H_
