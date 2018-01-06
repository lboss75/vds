#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include <map>
#include "binary_serialize.h"
#include "chunk_manager.h"
#include "guid.h"

namespace vds {
  namespace transactions {
    class transaction_block {
    public:

      template<typename item_type>
      void add(
          const guid & channel_id,
          item_type && item) {

        auto & s = this->channels_[channel_id];

        s << item_type::message_id;
        item.serialize(s);
      }

      void save(
          class database_transaction & t) const;

    private:

      std::map<guid, binary_serializer> channels_;

      void collect_dependencies(
          binary_serializer &s,
          class database_transaction &t,
          std::set<std::string> &ancestors) const;

      void register_transaction(
          class database_transaction &t,
          const std::set<std::string> &ancestors,
          const chunk_manager::chunk_info &block) const;

    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
