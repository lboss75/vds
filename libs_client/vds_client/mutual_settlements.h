//#ifndef __VDS_USER_MANAGER_mutual_settlements_H_
//#define __VDS_USER_MANAGER_mutual_settlements_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "database.h"
//#include "payment_transaction.h"
//
//namespace vds {
//  class user_manager;
//  class user_wallet;
//  namespace transactions {
//    class transaction_block;
//    class store_block_transaction;
//  }
//
//  class mutual_settlements : public std::enable_shared_from_this<mutual_settlements> {
//  public:
//    mutual_settlements();
//
//    expected<void> update(const transactions::transaction_block & block, const transactions::store_block_transaction & message);
//    expected<void> update(const transactions::transaction_block & block, const transactions::payment_transaction & message);
//    expected<void> update(const transactions::transaction_block & block, const transactions::payment_request_transaction & message);
//    expected<void> calculate(const service_provider * sp, const std::list<std::shared_ptr<user_wallet>>& wallets, database_read_transaction &t);
//
//  private:
//    struct request_info
//    {
//      std::chrono::system_clock::time_point created_time_;
//      transactions::payment_request_transaction transaction_;
//
//    };
//    struct payment_info
//    {
//      uint64_t block_size;
//      std::chrono::system_clock::time_point created_time_;
//      std::chrono::system_clock::time_point last_payment_time_;
//      std::map<const_data_buffer, request_info> payment_requests_;
//    };
//
//    std::map<const_data_buffer, payment_info> blocks_;
//    std::chrono::steady_clock::time_point last_calculate_;
//  };
//}
//
//#endif // __VDS_USER_MANAGER_mutual_settlements_H_
