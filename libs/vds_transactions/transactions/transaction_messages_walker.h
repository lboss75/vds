#ifndef __VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_
#define __VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "binary_serialize.h"
#include "channel_add_message_transaction.h"
#include "create_channel_transaction.h"
#include "payment_transaction.h"
#include "channel_message.h"

namespace vds {
  namespace transactions {

    template<typename implementation_class>
    class transaction_messages_walker {
    public:
      bool visit(const transactions::payment_transaction &message) {
        return true;
      }

      bool visit(const transactions::channel_message &message) {
        return true;
      }

      bool process(const const_data_buffer & message_data) {
        binary_deserializer s(message_data);

        while (0 < s.size()) {
          uint8_t message_id;
          s >> message_id;

          switch ((transaction_id) message_id) {
            case transactions::payment_transaction::message_id: {
              if (!static_cast<implementation_class *>(this)->visit(
                  transactions::payment_transaction(s))) {
                return false;
              }
              break;
            }
            case transactions::channel_message::message_id: {
              if (!static_cast<implementation_class *>(this)->visit(
                  transactions::channel_message(s))) {
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

    template <typename functor_type, typename functor_signature>
    class _functor_info;

    template <typename functor_type, typename result, typename class_name, typename arg_type>
    class _functor_info<functor_type, result(class_name::*)(arg_type) const>
    {
    public:
      typedef typename std::remove_const<typename std::remove_reference<arg_type>::type>::type argument_type;
    };

    template <typename functor_type, typename result, typename class_name, typename arg_type>
    class _functor_info<functor_type, result(class_name::*)(arg_type)>
    {
    public:
      typedef typename std::remove_const<typename std::remove_reference<arg_type>::type>::type argument_type;
    };

    template <typename functor_type>
    class functor_info : public _functor_info<functor_type, decltype(&functor_type::operator())>
    {};

    template <typename implementation_class, typename... handler_types>
    class _transaction_messages_walker_lambdas;

    template <typename implementation_class>
    class _transaction_messages_walker_lambdas<implementation_class>
      : public transaction_messages_walker<implementation_class>
    {
    public:
      _transaction_messages_walker_lambdas() {
      }
    };

    template <typename implementation_class, typename first_handler_type, typename... handler_types>
    class _transaction_messages_walker_lambdas<implementation_class, first_handler_type, handler_types...>
        : public _transaction_messages_walker_lambdas<implementation_class, handler_types...>
    {
      using base_class = _transaction_messages_walker_lambdas<implementation_class, handler_types...>;
    public:
      _transaction_messages_walker_lambdas(
          first_handler_type && first_handler,
          handler_types && ... handler)
      : first_handler_(std::forward<first_handler_type>(first_handler)),
          base_class(std::forward<handler_types>(handler)...) {
      }

      bool visit(const typename functor_info<first_handler_type>::argument_type & message){
        return this->first_handler_(message);
      }

    private:
      first_handler_type first_handler_;
    };

    template <typename... handler_types>
    class transaction_messages_walker_lambdas
        : public _transaction_messages_walker_lambdas<transaction_messages_walker_lambdas<handler_types...>, handler_types...>{
      using base_class = _transaction_messages_walker_lambdas<transaction_messages_walker_lambdas<handler_types...>, handler_types...>;
    public:
      transaction_messages_walker_lambdas(
        handler_types && ... handler)
      : base_class(std::forward<handler_types>(handler)...) {
      }
    };


  }
}

#endif //__VDS_TRANSACTIONS_TRANSACTION_MESSAGES_WALKER_H_

