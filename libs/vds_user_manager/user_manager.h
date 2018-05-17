#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>
#include <include/transaction_block.h>
#include "transactions/file_add_transaction.h"
#include "include/transaction_block_builder.h"
#include "user_channel.h"
#include "transactions/transaction_messages_walker.h"
#include "encoding.h"

namespace vds {
  class _user_manager;

  class user_manager : public std::enable_shared_from_this<user_manager>
  {
  public:
    enum class login_state_t {
      waiting_channel,
      login_sucessful,
      login_failed
    };

    user_manager();
    ~user_manager();

    async_task<> update(const service_provider & sp);
    login_state_t get_login_state() const;

    void load(
      const service_provider & sp,
      class database_transaction &t,
      const const_data_buffer & dht_user_id,
      const symmetric_key & user_password_key,
      const const_data_buffer& user_password_hash);

    member_user create_root_user(
      transactions::transaction_block_builder &log,
      class database_transaction &t,
      const std::string &user_name,
      const std::string &user_password,
      const asymmetric_private_key &private_key);

    async_task<vds::user_channel> create_channel(
      const service_provider &sp,
      user_channel::channel_type_t channel_type,
      const std::string &name) const;

    vds::user_channel create_channel(
        const service_provider &sp,
        transactions::transaction_block_builder &log,
        database_transaction &t,
        const vds::const_data_buffer &channel_id,
        user_channel::channel_type_t channel_type,
        const std::string &name,
        asymmetric_private_key &read_private_key,
        asymmetric_private_key &write_private_key) const;

    void reset(
        const service_provider &sp,
        class database_transaction &t,
        const std::string &root_user_name,
        const std::string &root_password,
        const asymmetric_private_key &root_private_key);

    async_task<> init_server(
      const vds::service_provider &sp,
      const std::string & root_user_name,
      const std::string & user_password,
      const std::string & device_name,
      int port);

    certificate get_channel_write_cert(const service_provider & sp, const const_data_buffer &channel_id) const;
    asymmetric_private_key get_channel_write_key(const service_provider & sp, const const_data_buffer &channel_id) const;
    asymmetric_private_key get_channel_write_key(const service_provider & sp, const const_data_buffer &channel_id, const std::string &cert_id) const;
    certificate get_channel_write_cert(const service_provider & sp, const const_data_buffer & channel_id, const std::string & cert_id) const;

    certificate get_channel_read_cert(const service_provider & sp, const const_data_buffer &channel_id) const;
    asymmetric_private_key get_channel_read_key(const service_provider & sp, const const_data_buffer &channel_id) const;
    asymmetric_private_key get_channel_read_key(const service_provider & sp, const const_data_buffer &channel_id, const std::string &cert_id) const;
    certificate get_certificate(const service_provider & sp, const std::string &cert_id) const;

    member_user import_user(const certificate &user_cert);

    user_channel get_channel(const service_provider & sp, const const_data_buffer &channel_id) const;
    std::list<user_channel> get_channels() const;

    template <typename... handler_types>
    void walk_messages(
        const service_provider & sp,
        const const_data_buffer & channel_id,
        database_transaction & t,
        handler_types && ... handlers) const{

      orm::transaction_log_record_dbo t1;
      auto st = t.get_reader(
          t1.select(t1.data)
              .where(
                  t1.state == (int)orm::transaction_log_record_dbo::state_t::validated
                  || t1.state == (int)orm::transaction_log_record_dbo::state_t::leaf
                  || t1.state == (int)orm::transaction_log_record_dbo::state_t::consensus)
              .order_by(t1.order_no));

      transactions::transaction_messages_walker_lambdas<handler_types...> channel_handlers(
          std::forward<handler_types>(handlers)...);
      transactions::transaction_messages_walker_lambdas<handler_types...> walker(
          [this, sp, channel_id, &channel_handlers](const transactions::channel_message & message){
            if(channel_id == message.channel_id()){

              auto read_cert_key = this->get_channel_read_key(
                  sp,
                  channel_id,
                  message.channel_read_cert_subject());
              const auto key_data = read_cert_key.decrypt(message.crypted_key());
              const auto key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data);
              const auto data = symmetric_decrypt::decrypt(key, message.crypted_data());

              return channel_handlers.process(data);
            }
            return true;
          });
      while (st.execute()) {
        transactions::transaction_block block(t1.data.get(st));
        if(!walker.process(block.block_messages())){
          break;
        }
      }
    }

    bool validate_and_save(
        const service_provider & sp,
        const std::list<certificate> &cert_chain);

    void save_certificate(const service_provider &sp, database_transaction &t, const certificate &cert);

    void create_root_user(
        const service_provider &sp,
        database_transaction &t,
        const std::string &user_email,
        const symmetric_key &password_key,
        const const_data_buffer &password_hash);

    const const_data_buffer &dht_user_id() const;

  private:
    std::unique_ptr<_user_manager> impl_;

    user_channel create_channel(
        const service_provider &sp,
        transactions::transaction_block_builder &log,
        database_transaction &t,
        const vds::const_data_buffer &channel_id,
        user_channel::channel_type_t channel_type,
        const std::string &name,
        const certificate &owner_cert,
        const asymmetric_private_key &owner_private_key,
        asymmetric_private_key &read_private_key,
        asymmetric_private_key &write_private_key) const;

    void save_certificate(const service_provider &sp, const certificate &cert);

  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
