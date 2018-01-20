#ifndef __VDS_USER_MANAGER_USER_CHANNEL_H_
#define __VDS_USER_MANAGER_USER_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <guid.h>
#include "const_data_buffer.h"

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
        const vds::certificate & read_cert,
        const vds::certificate & write_cert);


    const vds::guid &id() const;
    const vds::certificate & read_cert() const;
    const vds::certificate & write_cert() const;

  private:
    std::shared_ptr<class _user_channel> impl_;
  };
}
#endif // __VDS_USER_MANAGER_USER_CHANNEL_H_
