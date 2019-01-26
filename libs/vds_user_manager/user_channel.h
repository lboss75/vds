#ifndef __VDS_USER_MANAGER_USER_CHANNEL_H_
#define __VDS_USER_MANAGER_USER_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "const_data_buffer.h"
#include "transaction_block_builder.h"
#include "transaction_log_record_dbo.h"

namespace vds {
  class member_user;

  /*
   * Read:
   *    write_cert.check()
   *    read_private_key.decrypt(sym_key)
   *
   * Write:
   *    sym_key
   *    read_cert.crypt(sym_key)
   *    write_cert.sign(write_private_key)
   *
   * Add reader:
   *    send(write_cert + read_private_key)
   */
  class user_channel
  {
  public:
    user_channel(const user_channel &) = delete;
    user_channel & operator = (const user_channel &) = delete;

    class channel_type_t {
    public:
      static constexpr const char * personal_channel = "core.personal";
      static constexpr const char * notes_channel = "core.notes";
      static constexpr const char * news_channel = "core.news";
    };

    user_channel();
    user_channel(user_channel &&);

    user_channel(
      const const_data_buffer & id,
      const std::string & channel_type,
      const std::string & name,
      const std::shared_ptr<certificate> & read_cert,
      const std::shared_ptr<asymmetric_private_key> & read_private_key,
      const std::shared_ptr<certificate> & write_cert,
      const std::shared_ptr<asymmetric_private_key> & write_private_key);

    ~user_channel();

    const const_data_buffer &id() const;
    const std::string & channel_type() const;
    const std::string & name() const;
    expected<std::shared_ptr<certificate>> read_cert() const;
    expected<std::shared_ptr<certificate>> write_cert() const;

    expected<void> add_reader(
      transactions::transaction_block_builder& playback,
      const member_user& member_user,
      const vds::member_user& owner_user,
      const asymmetric_private_key& owner_private_key,
      const asymmetric_private_key& channel_read_private_key) const;

    expected<void> add_writer(
      transactions::transaction_block_builder& playback,
      const member_user& member_user,
      const vds::member_user& owner_user) const;

    class _user_channel * operator -> () const {
      return this->impl_;
    }

    std::shared_ptr<asymmetric_private_key> read_cert_private_key(const std::string& cert_subject);

    template<typename item_type>
    expected<void> add_log(
      transactions::transaction_block_builder & log,
      expected<item_type> && item) {

      CHECK_EXPECTED_ERROR(item);

      binary_serializer s;
      CHECK_EXPECTED(serialize(s, (uint8_t)item_type::message_id));

      _serialize_visitor v(s);
      item.value().visit(v);
      if(v.error()) {
        return unexpected(std::move(v.error()));
      }

      return this->add_to_log(log, s.get_buffer(), s.size());
    }

    user_channel & operator = (user_channel && other);

  private:
    class _user_channel * impl_;

    expected<void> add_to_log(
      transactions::transaction_block_builder & log,
      const uint8_t * data,
      size_t size);
  };
}

#endif // __VDS_USER_MANAGER_USER_CHANNEL_H_
