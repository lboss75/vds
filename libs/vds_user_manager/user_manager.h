#ifndef __VDS_USER_MANAGER_USER_MANAGER_H_
#define __VDS_USER_MANAGER_USER_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>
#include <stdafx.h>
#include <transactions/file_add_transaction.h>
#include "transaction_block.h"
#include "user_channel.h"
#include "user_invitation.h"
#include "transactions/channel_message_walker.h"

namespace vds {

  class user_manager
  {
  public:

    user_manager();

    void load(
      const service_provider & sp,
      class database_transaction &t,
      const guid & config_id);

    member_user create_root_user(
      transactions::transaction_block &log,
      class database_transaction &t,
      const guid &common_channel_id,
      const std::string &user_name,
      const std::string &user_password,
      const asymmetric_private_key &private_key);

    user_channel create_channel(
      const service_provider & sp,
      transactions::transaction_block &log, database_transaction &t,
      const vds::guid &channel_id, const std::string &name,
      const vds::guid &owner_id, const certificate &owner_cert,
      const asymmetric_private_key &owner_private_key,
      asymmetric_private_key &read_private_key,
      asymmetric_private_key &write_private_key) const;

    user_invitation reset(const service_provider &sp, class database_transaction &t, const std::string &root_user_name,
      const std::string &root_password, const asymmetric_private_key &root_private_key,
      const std::string &device_name, int port);

    async_task<> init_server(
      const vds::service_provider &sp,
      const user_invitation & request,
      const std::string & user_password,
      const std::string &device_name,
      int port);

    member_user lock_to_device(
      const service_provider &sp,
      class database_transaction &t,
      transactions::transaction_block & playback,
      const std::list<certificate> & certificate_chain,
      const member_user &user,
      const std::string &user_name,
      const std::string &user_password,
      const asymmetric_private_key &user_private_key,
      const std::string &device_name,
      const asymmetric_private_key & device_private_key,
      int port);

    member_user get_current_device(
      const service_provider &sp,
      asymmetric_private_key & device_private_key) const;

    certificate get_channel_write_cert(const service_provider & sp, const guid &channel_id) const;
    asymmetric_private_key get_channel_write_key(const service_provider & sp, const guid &channel_id) const;
    asymmetric_private_key get_channel_write_key(const service_provider & sp, const guid &channel_id, const guid &cert_id) const;
    certificate get_channel_write_cert(const service_provider & sp, const guid & channel_id, const guid & cert_id) const;

    certificate get_channel_read_cert(const service_provider & sp, const guid &channel_id) const;
    asymmetric_private_key get_channel_read_key(const service_provider & sp, const guid &channel_id) const;
    asymmetric_private_key get_channel_read_key(const service_provider & sp, const guid &channel_id, const guid &cert_id) const;
    certificate get_certificate(const service_provider & sp, const guid &cert_id) const;

    member_user import_user(const certificate &user_cert);

    user_channel get_channel(const service_provider & sp, const guid &channel_id) const;

    template <typename... handler_types>
    void walk_messages(
        const service_provider & sp,
        const guid & channel_id,
        database_transaction & t,
        handler_types && ... handlers) const{

      orm::transaction_log_record_dbo t1;
      auto st = t.get_reader(
          t1.select(t1.data)
              .where(t1.channel_id == channel_id)
              .order_by(t1.order_no));

      transactions::channel_message_walker_lambdas<handler_types...> walker(
          std::forward<handler_types>(handlers)...);
      while (st.execute()) {
        guid channel_id;
        uint64_t order_no;
        guid read_cert_id;
        guid write_cert_id;
        std::set<const_data_buffer> ancestors;
        const_data_buffer crypted_data;
        const_data_buffer crypted_key;
        const_data_buffer signature;

        transactions::transaction_block::parse_block(t1.data.get(st), channel_id, order_no, read_cert_id,
                                                     write_cert_id, ancestors,
                                                     crypted_data, crypted_key, signature);

        auto write_cert = this->get_channel_write_cert(sp, write_cert_id);
        if (!transactions::transaction_block::validate_block(write_cert, channel_id, order_no, read_cert_id,
                                                             write_cert_id, ancestors,
                                                             crypted_data, crypted_key, signature)) {
          throw std::runtime_error("Write signature error");
        }

        auto read_cert_key = this->get_channel_read_key(sp, channel_id, write_cert_id);
        const auto key_data = read_cert_key.decrypt(crypted_key);
        const auto key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data);
        const auto data = symmetric_decrypt::decrypt(key, crypted_data);

        binary_deserializer s(data);

        uint8_t message_id;
        s >> message_id;

        bool bContinue = true;
        switch ((transactions::transaction_id)message_id) {
          case transactions::transaction_id::file_add_transaction:
            bContinue = walker.visit(transactions::file_add_transaction(s));
            break;
        }

        if(!bContinue){
          break;
        }
      }
    }

    bool validate_and_save(
        const service_provider & sp,
        const std::list<certificate> &cert_chain);

    void save_certificate(const service_provider &sp, database_transaction &t, const certificate &cert);

  private:
    std::shared_ptr<class _user_manager> impl_;

  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_H_
