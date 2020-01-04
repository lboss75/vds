///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#ifndef __VDS_TRANSACTIONS_TRANSACTION_STATE_CALCULATOR_H_
//#define __VDS_TRANSACTIONS_TRANSACTION_STATE_CALCULATOR_H_
//
//#include <database.h>
//#include "include/transaction_record_state.h"
//
//namespace vds {
//  namespace transactions {
//    class transaction_state_calculator {
//    public:
//      static transaction_record_state calculate(
//          struct database_read_transaction &t,
//          const std::set<const_data_buffer> & ancestors,
//          const std::chrono::system_clock::time_point & time_point,
//          uint64_t max_order_no);
//
//    private:
//      enum class log_state_t {
//        target,
//        include,
//        exclude
//      };
//
//      struct log_state {
//        log_state_t state_;
//        const_data_buffer block_body_;
//      };
//
//      std::map<uint64_t/*order_no*/, std::map<const_data_buffer/*log_id*/, log_state /*state*/>> items_;
//
//      void add_ancestor(
//          vds::database_read_transaction &t,
//          const const_data_buffer & ancestor_id);
//
//      transaction_record_state process(
//        database_read_transaction &t);
//
//      transaction_record_state & load_state(
//        database_read_transaction &t,
//        const const_data_buffer & log_id);
//
//      void resolve(
//          database_read_transaction &t,
//          const uint64_t order_no,
//          const const_data_buffer &node_id);
//
//      static const_data_buffer looking_leaf(
//        database_read_transaction &t,
//        const const_data_buffer & log_id);
//
//      static transaction_record_state calculate_state(
//        database_read_transaction& t,
//        const const_data_buffer& log_id);
//
//      void set_state(
//        uint64_t order_no,
//        const vds::const_data_buffer &log_id,
//        log_state_t state);
//
//    };
//  }
//}
//
//#endif //__VDS_TRANSACTIONS_TRANSACTION_STATE_CALCULATOR_H_
