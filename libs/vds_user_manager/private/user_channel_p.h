#ifndef __VDS_USER_MANAGER_USER_CHANNEL_P_H_
#define __VDS_USER_MANAGER_USER_CHANNEL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "member_user.h"

namespace vds {

  class _user_channel
  {
  public:
      _user_channel();
      _user_channel(
        const vds::guid &id,
		const std::string & name,
        const vds::certificate & read_cert,
        const vds::certificate & write_cert);

    const vds::guid &id() const { return this->id_;}
    const vds::certificate & read_cert() const { return this->read_cert_; }
    const vds::certificate & write_cert() const { return this->write_cert_; }
	  void add_reader(
		  transactions::transaction_block& playback,
		  const member_user& member_user,
		  const vds::member_user& owner_user,
		  const asymmetric_private_key& owner_private_key,
		  const asymmetric_private_key& channel_read_private_key) const;

	  void add_writer(
		  transactions::transaction_block& playback,
		  const member_user& member_user,
		  const vds::member_user& owner_user,
		  const asymmetric_private_key& owner_private_key,
		  const asymmetric_private_key& channel_write_private_key) const;

  private:
    guid id_;
	std::string name_;
    certificate read_cert_;
    certificate write_cert_;
  };
}

#endif // __VDS_USER_MANAGER_USER_CHANNEL_P_H_
