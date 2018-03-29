#ifndef __VDS_USER_MANAGER_CHANNEL_MESSAGE_WALKER_H_
#define __VDS_USER_MANAGER_CHANNEL_MESSAGE_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "binary_serialize.h"
#include "channel_add_message_transaction.h"
#include "create_channel_transaction.h"

namespace vds {
  namespace transactions {

    template<typename implementation_class>
    class channel_message_walker {
    public:
      bool visit(const transactions::create_channel_transaction &message) {
        return true;
      }

      bool visit(const channel_add_message_transaction::add_device_user &message) {
        return true;
      }

      void process(const const_data_buffer & message_data) {
        binary_deserializer s(message_data);

        for (;;) {
          uint8_t message_id;
          s >> message_id;

          switch(message_id) {
          case channel_add_message_transaction::add_device_user::message_id: {
            if (!static_cast<implementation_class *>(this)->visit(channel_add_message_transaction::add_device_user(s))) {
              return;
            }
            break;
          }
          default: {
            throw std::runtime_error("Invalid message");
          }
          }
        }
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

    template <typename... handler_types>
    class channel_message_walker_lambdas;

    template <>
    class channel_message_walker_lambdas<>
    {
    public:
      channel_message_walker_lambdas() {
      }
    };

    template <typename first_handler_type, typename... handler_types>
    class channel_message_walker_lambdas<first_handler_type, handler_types...>
        : public channel_message_walker_lambdas<handler_types...>
    {
      using base_class = channel_message_walker_lambdas<handler_types...>;
    public:
      channel_message_walker_lambdas(
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

  }
}

#endif //__VDS_USER_MANAGER_CHANNEL_MESSAGE_WALKER_H_

