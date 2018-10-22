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
  class user_wallet;
  class _user_manager;

  class user_manager : public std::enable_shared_from_this<user_manager>
  {
  public:
    enum class login_state_t {
      waiting,
      login_sucessful,
      login_failed
    };

    user_manager(const service_provider * sp);
    ~user_manager();

    login_state_t get_login_state() const;

    vds::async_task<void> update();

    void load(
      
      class database_transaction &t,
      const std::string & user_login,
      const std::string & user_password);

    vds::async_task<vds::user_channel> create_channel(
      
      const std::string &name) const;

    void reset(
        
        const std::string &root_user_name,
        const std::string &root_password,
        const cert_control::private_info_t & private_info);

    //vds::async_task<void> init_server(
    //  
    //  const std::string & root_user_name,
    //  const std::string & user_password,
    //  const std::string & device_name,
    //  int port);

    std::shared_ptr<user_channel> get_channel( const const_data_buffer &channel_id) const;
    std::map<const_data_buffer, std::shared_ptr<user_channel>> get_channels() const;

    template <typename... handler_types>
    void walk_messages(
      
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
          [this, channel_id, &channel_handlers](const transactions::channel_message & message)->bool {
          if (channel_id == message.channel_id()) {

            auto read_cert_key = this->get_channel(channel_id)->read_cert_private_key(message.channel_read_cert_subject());
            const auto key_data = read_cert_key->decrypt(message.crypted_key());
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
        
        const std::list<std::shared_ptr<certificate>> &cert_chain);

    void save_certificate( database_transaction &t, const certificate &cert);

    member_user get_current_user() const;
    const std::shared_ptr<asymmetric_private_key> & get_current_user_private_key() const;
    const std::string & user_name() const;

    static vds::async_task<const_data_buffer> create_register_request(
      const service_provider * sp,
      const std::string& userName,
      const std::string& userEmail,
      const std::string& userPassword);

    static bool parse_join_request(
        
        const const_data_buffer & data,
        std::string & userName,
        std::string & userEmail);

    vds::async_task<bool> approve_join_request(
      
      const const_data_buffer & data);

    const std::list<std::shared_ptr<user_wallet>> & wallets() const;

  private:
    const service_provider * sp_;
    std::shared_ptr<_user_manager> impl_;

    vds::async_task<void> save_certificate( const std::shared_ptr<certificate> &cert);
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
