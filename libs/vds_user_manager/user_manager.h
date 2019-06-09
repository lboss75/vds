#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>
#include "transaction_block.h"
#include "transaction_block_builder.h"
#include "user_channel.h"
#include "transaction_messages_walker.h"
#include "encoding.h"
#include "transaction_log_record_dbo.h"

namespace vds {
  class user_wallet;
  class _user_manager;
  class member_user;

  class user_manager : public std::enable_shared_from_this<user_manager>
  {
  public:
    enum class login_state_t {
      waiting,
      login_successful,
      login_failed
    };

    user_manager(const service_provider * sp);
    ~user_manager();

    login_state_t get_login_state() const;

    vds::async_task<vds::expected<void>> update();

    vds::expected<void> update(database_read_transaction & t) const;

    expected<void> load(
      class database_read_transaction &t,
      const std::string & user_login,
      const std::string & user_password);

    vds::async_task<vds::expected<vds::user_channel>> create_channel(
      const std::string & channel_type,
      const std::string &name) const;

    expected<void> reset(
        
        const std::string &root_user_name,
        const std::string &root_password,
        const cert_control::private_info_t & private_info);

    //vds::async_task<vds::expected<void>> init_server(
    //  
    //  const std::string & root_user_name,
    //  const std::string & user_password,
    //  const std::string & device_name,
    //  int port);

    std::shared_ptr<user_channel> get_channel( const const_data_buffer &channel_id) const;
    std::map<const_data_buffer, std::shared_ptr<user_channel>> get_channels() const;

    template <typename... handler_types>
    expected<void> walk_messages(

      const const_data_buffer & channel_id,
      database_read_transaction & t,
      handler_types && ... handlers) const {

      orm::transaction_log_record_dbo t1;
      GET_EXPECTED(st, t.get_reader(
        t1.select(t1.data)
        .where(
          t1.state == orm::transaction_log_record_dbo::state_t::processed
          || t1.state == orm::transaction_log_record_dbo::state_t::leaf)
        .order_by(t1.order_no)));

      transactions::channel_messages_walker_lambdas<handler_types...> channel_handlers(
        std::forward<handler_types>(handlers)...);
      WHILE_EXPECTED(st.execute()) {
        GET_EXPECTED(block, transactions::transaction_block::create(t1.data.get(st)));
        if (!block.walk_messages(
          [this, channel_id, &channel_handlers, tp = block.time_point()](const transactions::channel_message & message)->expected<bool> {
          if (channel_id == message.channel_id()) {

            auto read_cert_key = this->get_channel(channel_id)->read_cert_private_key(message.channel_read_cert_subject());
            GET_EXPECTED(key_data, read_cert_key->decrypt(message.crypted_key()));
            GET_EXPECTED(key, symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data));
            GET_EXPECTED(data, symmetric_decrypt::decrypt(key, message.crypted_data()));

            return channel_handlers.process(this->sp_, data, transactions::message_environment_t { tp });
          }

          return true;
        })) {
          break;
        }
      }
      WHILE_EXPECTED_END()

        return expected<void>();
    }

    expected<uint64_t> get_device_storage_used();
    expected<uint64_t> get_device_storage_size();
    expected<uint64_t> get_user_balance();

    //expected<bool> validate_and_save(
    //    
    //    const std::list<std::shared_ptr<certificate>> &cert_chain);

    //static expected<void> save_certificate( database_transaction &t, const asymmetric_public_key &cert);

    member_user get_current_user() const;
    const std::shared_ptr<asymmetric_private_key> & get_current_user_private_key() const;
    const std::string & user_name() const;

    static async_task<expected<void>> create_user(
      const service_provider * sp,
      const std::string& userName,
      const std::string& userEmail,
      const std::string& userPassword);

    const std::list<std::shared_ptr<user_wallet>> & wallets() const;

  private:
    const service_provider * sp_;
    std::shared_ptr<_user_manager> impl_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
