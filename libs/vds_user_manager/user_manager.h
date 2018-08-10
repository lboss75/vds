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

namespace vds {
  class _user_manager;

  class user_manager : public std::enable_shared_from_this<user_manager>
  {
  public:
    enum class login_state_t {
      waiting,
      login_sucessful,
      login_failed
    };

    user_manager();
    ~user_manager();

    login_state_t get_login_state() const;

    async_task<> update(const service_provider & sp);

    void load(
      const service_provider & sp,
      class database_transaction &t,
      const std::string &user_credentials_key,
      const asymmetric_private_key & user_private_key);

    async_task<vds::user_channel> create_channel(
      const service_provider &sp,
      const std::string &name) const;

    void reset(
        const service_provider &sp,
        const std::string &root_user_name,
        const std::string &root_password,
        const cert_control::private_info_t & private_info);

    //async_task<> init_server(
    //  const vds::service_provider &sp,
    //  const std::string & root_user_name,
    //  const std::string & user_password,
    //  const std::string & device_name,
    //  int port);

    user_channel get_channel(const service_provider & sp, const const_data_buffer &channel_id) const;
    std::map<const_data_buffer, user_channel> get_channels() const;

    template <typename... handler_types>
    void walk_messages(
      const service_provider & sp,
      const const_data_buffer & channel_id,
      database_transaction & t,
      handler_types && ... handlers) const {

      orm::transaction_log_record_dbo t1;
      auto st = t.get_reader(
        t1.select(t1.data)
        .where(
          t1.state == orm::transaction_log_record_dbo::state_t::processed
          || t1.state == orm::transaction_log_record_dbo::state_t::leaf)
        .order_by(t1.order_no));

      transactions::channel_messages_walker_lambdas<handler_types...> channel_handlers(
        std::forward<handler_types>(handlers)...);
      while (st.execute()) {
        transactions::transaction_block block(t1.data.get(st));
        if (!block.walk_messages(
          [this, sp, channel_id, &channel_handlers](const transactions::channel_message & message)->bool {
          if (channel_id == message.channel_id()) {

            auto read_cert_key = this->get_channel(sp, channel_id).read_cert_private_key(message.channel_read_cert_subject());
            const auto key_data = read_cert_key.decrypt(message.crypted_key());
            const auto key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data);
            const auto data = symmetric_decrypt::decrypt(key, message.crypted_data());

            return channel_handlers.process(data);
          }
          return true;
        })) {
          break;
        }
      }
    }

    bool validate_and_save(
        const service_provider & sp,
        const std::list<certificate> &cert_chain);

    void save_certificate(const service_provider &sp, database_transaction &t, const certificate &cert);

    member_user get_current_user() const;
    const asymmetric_private_key & get_current_user_private_key() const;
    const std::string & user_name() const;

    static async_task<const_data_buffer> create_register_request(
      const service_provider& sp,
      const std::string& userName,
      const std::string& userEmail,
      const std::string& userPassword);

    static bool parse_join_request(
        const service_provider& sp,
        const const_data_buffer & data,
        std::string & userName,
        std::string & userEmail);

    async_task<bool> approve_join_request(
      const service_provider& sp,
      const const_data_buffer & data);

  private:
    std::shared_ptr<_user_manager> impl_;

    void save_certificate(const service_provider &sp, const certificate &cert);
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
