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
    enum class channel_type_t {
      personal_channel,
      notes_channel,
      inter_person,
      news_channel,
      files_channel
    };

    user_channel();

    user_channel(
      const const_data_buffer & id,
      channel_type_t channel_type,
      const std::string & name,
      const vds::certificate & read_cert,
      const asymmetric_private_key & read_private_key,
      const vds::certificate & write_cert,
      const asymmetric_private_key & write_private_key);


    const const_data_buffer &id() const;
    channel_type_t channel_type() const;
    const std::string & name() const;
    const vds::certificate & read_cert() const;
    const vds::certificate & write_cert() const;

    void add_reader(
      transactions::transaction_block_builder& playback,
      const member_user& member_user,
      const vds::member_user& owner_user,
      const asymmetric_private_key& owner_private_key,
      const asymmetric_private_key& channel_read_private_key) const;

    void add_writer(
      transactions::transaction_block_builder& playback,
      const member_user& member_user,
      const vds::member_user& owner_user) const;

    bool operator !() const {
      return nullptr == this->impl_.get();
    }

    operator bool () const {
      return nullptr != this->impl_.get();
    }

    class _user_channel * operator -> () const {
      return this->impl_.get();
    }

    asymmetric_private_key read_cert_private_key(const std::string& cert_subject);

    template<typename item_type>
    void add_log(
      transactions::transaction_block_builder & log,
      item_type && item) {

      binary_serializer s;
      s
        << (uint8_t)item_type::message_id;
      item.serialize(s);

      this->add_to_log(log, s.data().data(), s.size());
    }

  private:
    std::shared_ptr<class _user_channel> impl_;

    void add_to_log(
      transactions::transaction_block_builder & log,
      const uint8_t * data,
      size_t size);
  };

  inline user_channel::channel_type_t string2channel_type(const std::string & val) {
    if ("p" == val) {
      return vds::user_channel::channel_type_t::personal_channel;
    }
    if ("n" == val) {
      return vds::user_channel::channel_type_t::notes_channel;
    }
    if ("c" == val) {
      return vds::user_channel::channel_type_t::inter_person;
    }
    if ("N" == val) {
      return vds::user_channel::channel_type_t::news_channel;
    }
    if("f" == val) {
      return vds::user_channel::channel_type_t::files_channel;
    }

    throw std::runtime_error("Invalid value");
  }
}
namespace std {
  inline string to_string(vds::user_channel::channel_type_t val) {
    switch (val) {
    case vds::user_channel::channel_type_t::personal_channel: {
      return "p";
    }
    case vds::user_channel::channel_type_t::notes_channel: {
      return "n";
    }
    case vds::user_channel::channel_type_t::inter_person: {
      return "c";
    }
    case vds::user_channel::channel_type_t::news_channel: {
      return "N";
    }
    case vds::user_channel::channel_type_t::files_channel: {
      return "f";
    }

    default: {
      throw std::runtime_error("Invalid value");
    }
    }
  }
}

#endif // __VDS_USER_MANAGER_USER_CHANNEL_H_
