#ifndef __VDS_USER_MANAGER_USER_CHANNEL_H_
#define __VDS_USER_MANAGER_USER_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <guid.h>
#include "const_data_buffer.h"
#include "transaction_block.h"
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
    user_channel();

    user_channel(
        const guid & id,
		const std::string & name,
		const vds::certificate & read_cert,
        const vds::certificate & write_cert);


    const vds::guid &id() const;
    const vds::certificate & read_cert() const;
    const vds::certificate & write_cert() const;

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

		bool operator !() const {
			return nullptr == this->impl_.get();
		}


  private:
    std::shared_ptr<class _user_channel> impl_;
  };
}
#endif // __VDS_USER_MANAGER_USER_CHANNEL_H_
