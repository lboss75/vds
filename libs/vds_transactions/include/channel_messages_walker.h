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
#include "logger.h"

namespace vds {
  namespace transactions {
    struct message_environment_t {
      std::chrono::system_clock::time_point time_point_;
      std::string user_name_;
    };

    class channel_messages_walker {
    public:
      virtual expected<bool> visit(const channel_add_reader_transaction & /*message*/, const message_environment_t & /*message_environment*/) {
        return true;
      }

      virtual expected<bool> visit(const channel_add_writer_transaction & /*message*/, const message_environment_t & /*message_environment*/) {
        return true;
      }

      virtual expected<bool> visit(const channel_create_transaction & /*message*/, const message_environment_t & /*message_environment*/) {
        return true;
      }

      virtual expected<bool> visit(const user_message_transaction & /*message*/, const message_environment_t & /*message_environment*/) {
        return true;
      }

      virtual expected<bool> visit(const control_message_transaction & /*message*/, const message_environment_t & /*message_environment*/) {
        return true;
      }

      expected<bool> process(
              const service_provider * sp,
              const const_data_buffer & message_data,
              const message_environment_t & message_environment) {
        binary_deserializer s(message_data);

        //std::string log;
        //for(size_t i = 0; i < message_data.size(); ++i){
        //    char buf[3];
        //    sprintf(buf, "%02X", message_data[i]);
        //    log += buf;
        //}
        //sp->get<logger>()->debug("UserMng", "Process message [%s]", log.c_str());

        while(0 < s.size()) {
          uint8_t message_id;
          CHECK_EXPECTED(deserialize(s, message_id));

          sp->get<logger>()->debug("UserMng", "Process message %d", message_id);

          switch((channel_message_id)message_id) {
          case channel_add_reader_transaction::message_id: {
            GET_EXPECTED(message, message_deserialize<channel_add_reader_transaction>(s));
            GET_EXPECTED(result, this->visit(message, message_environment));
            if (!result) {
              return false;
            }
            break;
          }
            case channel_add_writer_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<channel_add_writer_transaction>(s));
              GET_EXPECTED(result, this->visit(message, message_environment));
              if (!result) {
                return false;
              }
              break;
            }
            case channel_create_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<channel_create_transaction>(s));
              GET_EXPECTED(result, this->visit(message, message_environment));
              if (!result) {
                return false;
              }
              break;
            }
            case user_message_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<user_message_transaction>(s));
              GET_EXPECTED(result, this->visit(message, message_environment));
              if (!result) {
                return false;
              }
              break;
            }
            case control_message_transaction::message_id: {
              GET_EXPECTED(message, message_deserialize<control_message_transaction>(s));
              GET_EXPECTED(result, this->visit(message, message_environment));
              if (!result) {
                return false;
              }
              break;
            }
            default: {
            return vds::make_unexpected<std::runtime_error>("Invalid channel message " + std::to_string(message_id));
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

      expected<bool> visit(
        const typename std::tuple_element_t<0, typename functor_info<first_handler_type>::arguments_tuple> & message,
        const message_environment_t & message_environment) override {
        return this->first_handler_(message, message_environment);
      }

    private:
      first_handler_type first_handler_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_MESSAGES_WALKER_H_

