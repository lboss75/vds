#ifndef __VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_
#define __VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "binary_serialize.h"
#include "payment_transaction.h"
#include "root_user_transaction.h"
#include "channel_message.h"
#include "create_user_transaction.h"

namespace vds {
  namespace transactions {

    class transaction_messages_walker {
    public:
      virtual bool visit(const payment_transaction &/*message*/) {
        return true;
      }

      virtual bool visit(const root_user_transaction &/*message*/) {
        return true;
      }

      virtual bool visit(const channel_message &message) {
        return true;
      }

      virtual bool visit(const create_user_transaction &message) {
        return true;
      }

      bool process(const const_data_buffer & message_data) {
        binary_deserializer s(message_data);

        while (0 < s.size()) {
          uint8_t message_id;
          s >> message_id;

          switch ((transaction_id) message_id) {
            case transactions::payment_transaction::message_id: {
              if (!this->visit(payment_transaction(s))) {
                return false;
              }
              break;
            }
            case transactions::root_user_transaction::message_id: {
              if (!this->visit(root_user_transaction(s))) {
                return false;
              }
              break;
            }
            case transactions::create_user_transaction::message_id: {
              if (!this->visit(create_user_transaction(s))) {
                return false;
              }
              break;
            }
            case transactions::channel_message::message_id: {
              if (!this->visit(channel_message(s))) {
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
    class transaction_messages_walker_lambdas;

    template <>
    class transaction_messages_walker_lambdas<>
      : public transaction_messages_walker
    {
    public:
      transaction_messages_walker_lambdas() {
      }
    };

    template <typename first_handler_type, typename... handler_types>
    class transaction_messages_walker_lambdas<first_handler_type, handler_types...>
        : public transaction_messages_walker_lambdas<handler_types...>
    {
      using base_class = transaction_messages_walker_lambdas<handler_types...>;
    public:
      transaction_messages_walker_lambdas(
          first_handler_type && first_handler,
          handler_types && ... handler)
      : base_class(std::forward<handler_types>(handler)...),
        first_handler_(std::forward<first_handler_type>(first_handler)) {
      }

      bool visit(const typename functor_info<first_handler_type>::argument_type & message) override {
        return this->first_handler_(message);
      }

    private:
      first_handler_type first_handler_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_

