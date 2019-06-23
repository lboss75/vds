#ifndef __VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_
#define __VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "binary_serialize.h"
#include "payment_transaction.h"
#include "channel_message.h"
#include "create_user_transaction.h"
#include "node_manager_transactions.h"

namespace vds {
  namespace transactions {

    class transaction_messages_walker {
    public:
      virtual expected<bool> visit(const payment_transaction &/*message*/) {
        return true;
      }

      virtual expected<bool> visit(const channel_message & /*message*/) {
        return true;
      }

      virtual expected<bool> visit(const create_user_transaction & /*message*/) {
        return true;
      }

      virtual expected<bool> visit(const node_add_transaction & /*message*/) {
        return true;
      }

      virtual expected<bool> visit(const create_wallet_transaction & /*message*/) {
        return true;
      }

      virtual expected<bool> visit(const asset_issue_transaction & /*message*/) {
        return true;
      }

      expected<bool> process(const const_data_buffer & message_data) {
        binary_deserializer s(message_data);

        while (0 < s.size()) {
          uint8_t message_id;
          CHECK_EXPECTED(s >> message_id);

          switch ((transaction_id) message_id) {
            case transactions::payment_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<payment_transaction>(s));
              GET_EXPECTED(result, this->visit(message));
              if (!result) {
                return false;
              }
              break;
            }
            case transactions::create_user_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<create_user_transaction>(s));
              GET_EXPECTED(result, this->visit(message));
              if (!result) {
                return false;
              }
              break;
            }
            case transactions::channel_message::message_id: {
              GET_EXPECTED(message, message_deserialize<channel_message>(s));
              GET_EXPECTED(result, this->visit(message));
              if (!result) {
                return false;
              }
              break;
            }
            case transactions::node_add_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<node_add_transaction>(s));
              GET_EXPECTED(result, this->visit(message));
              if (!result) {
                return false;
              }
              break;
            }
            case transactions::create_wallet_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<create_wallet_transaction>(s));
              GET_EXPECTED(result, this->visit(message));
              if (!result) {
                return false;
              }
              break;
            }
            case transactions::asset_issue_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<asset_issue_transaction>(s));
              GET_EXPECTED(result, this->visit(message));
              if (!result) {
                return false;
              }
              break;
            }
            default: {
              return vds::make_unexpected<std::runtime_error>("Invalid message " + std::to_string(message_id));
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

      expected<bool> visit(const typename functor_info<first_handler_type>::argument_type & message) override {
        return this->first_handler_(message);
      }

    private:
      first_handler_type first_handler_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_

