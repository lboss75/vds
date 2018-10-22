#ifndef __VDS_TRANSACTIONS_CHANNEL_MESSAGES_WALKER_H_
#define __VDS_TRANSACTIONS_CHANNEL_MESSAGES_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "binary_serialize.h"
#include "payment_transaction.h"
#include "channel_message_id.h"
#include "channel_add_reader_transaction.h"
#include "channel_add_writer_transaction.h"
#include "channel_create_transaction.h"
#include "user_message_transaction.h"
#include "func_utils.h"
#include "control_message_transaction.h"

namespace vds {
  namespace transactions {
    class channel_messages_walker {
    public:
      virtual bool visit(const channel_add_reader_transaction & /*message*/) {
        return true;
      }

      virtual bool visit(const channel_add_writer_transaction & /*message*/) {
        return true;
      }

      virtual bool visit(const channel_create_transaction & /*message*/) {
        return true;
      }

      virtual bool visit(const user_message_transaction & /*message*/) {
        return true;
      }

      virtual bool visit(const control_message_transaction & /*message*/) {
        return true;
      }

      bool process(const const_data_buffer & message_data) {
        binary_deserializer s(message_data);

        while(0 < s.size()) {
          uint8_t message_id;
          s >> message_id;

          switch((channel_message_id)message_id) {
          case channel_add_reader_transaction::message_id: {
            if (!this->visit(message_deserialize<channel_add_reader_transaction>(s))) {
              return false;
            }
            break;
          }
            case channel_add_writer_transaction::message_id: {
              if (!this->visit(message_deserialize<channel_add_writer_transaction>(s))) {
                return false;
              }
              break;
            }
            case channel_create_transaction::message_id: {
              if (!this->visit(message_deserialize<channel_create_transaction>(s))) {
                return false;
              }
              break;
            }
            case user_message_transaction::message_id: {
              if (!this->visit(message_deserialize<user_message_transaction>(s))) {
                return false;
              }
              break;
            }
            case control_message_transaction::message_id: {
              if (!this->visit(message_deserialize<control_message_transaction>(s))) {
                return false;
              }
              break;
            }
            default: {
            throw std::runtime_error("Invalid message");
          }
          }
        }

        return true;
      }
    };

    template <typename... handler_types>
    class channel_messages_walker_lambdas;

    template <>
    class channel_messages_walker_lambdas<>
      : public channel_messages_walker
    {
    public:
      channel_messages_walker_lambdas() {
      }
    };

    template <typename first_handler_type, typename... handler_types>
    class channel_messages_walker_lambdas<first_handler_type, handler_types...>
        : public channel_messages_walker_lambdas<handler_types...>
    {
      using base_class = channel_messages_walker_lambdas<handler_types...>;
    public:
      channel_messages_walker_lambdas(
          first_handler_type && first_handler,
          handler_types && ... handler)
      : base_class(std::forward<handler_types>(handler)...),
        first_handler_(std::forward<first_handler_type>(first_handler)) {
      }

      bool visit(const typename functor_info<first_handler_type>::argument_type & message) override{
        return this->first_handler_(message);
      }

    private:
      first_handler_type first_handler_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_MESSAGES_WALKER_H_

