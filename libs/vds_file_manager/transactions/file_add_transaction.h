#ifndef __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
#define __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "transaction_log.h"
#include "binary_serialize.h"

namespace vds {
  namespace file_manager_transactions {

    class file_add_transaction {
    public:
      static const uint8_t category_id = transaction_log::file_manager_category_id;
      static const uint8_t message_id = 'a';

      file_add_transaction(
          const std::string &name,
          const std::string &mimetype,
          const std::list<std::string> & file_blocks)
      : name_(name), mimetype_(mimetype), file_blocks_(file_blocks) {
      }

      binary_serializer & serialize(binary_serializer & s) const {
        return s;
      }

      const std::string & name () const {
        return this->name_;
      }

      const std::string &mimetype() const {
        return this->mimetype_;
      }

      const std::list<std::string> & file_blocks() const {
        return this->file_blocks_;
      }

    private:
      std::string name_;
      std::string mimetype_;
      std::list<std::string> file_blocks_;
    };

  }
}
#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
