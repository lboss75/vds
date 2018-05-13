/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include "include/transaction_record_state.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"

vds::transactions::transaction_record_state
vds::transactions::transaction_record_state::calculate(
    struct database_transaction &t,
    std::set<vds::const_data_buffer> ancestors) {

  std::map<uint64_t, std::list<const_data_buffer>> records;

  for(const auto & ancestor : ancestors){
    orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(
        t1.select(
            t1.order_no,
            t1.state_data)
        .where(t1.id == base64::from_bytes(ancestor)));
    if(!st.execute()){
      throw std::runtime_error("Invalid data");
    }

    auto state_data = t1.state_data.get(st);
    binary_deserializer s(state_data);



  }

}
