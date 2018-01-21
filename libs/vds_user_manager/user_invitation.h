#ifndef __VDS_USER_MANAGER_USER_INVITATION_H_
#define __VDS_USER_MANAGER_USER_INVITATION_H_
#include "member_user.h"
#include "database.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class user_invitation
  {
  public:
	  user_invitation() = default;
	  user_invitation(
			const guid & user_id,
			const std::string & user_name,
			const certificate & user_certificate,
			const asymmetric_private_key & user_private_key,
			const guid & common_channel_id,
			const certificate & common_channel_read_cert,
			const asymmetric_private_key & common_channel_private_key,
		  const certificate & common_channel_write_cert,
		  const asymmetric_private_key & common_channel_write_private_key,
		  const std::list<certificate> & certificate_chain)
		  :user_id_(user_id),
		  user_name_(user_name),
		  user_certificate_(user_certificate),
		  user_private_key_(user_private_key),
		  common_channel_id_(common_channel_id),
		  common_channel_read_cert_(common_channel_read_cert),
		  common_channel_read_private_key_(common_channel_private_key),
		  common_channel_write_cert_(common_channel_write_cert),
		  common_channel_write_private_key_(common_channel_write_private_key),
		  certificate_chain_(certificate_chain)

	  {		  
	  }

	  member_user get_user() const;
	  const asymmetric_private_key & get_user_private_key() const
	  {
		  return this->user_private_key_;		  
	  }

	  const guid & common_channel_id() const
	  {
		  return this->common_channel_id_;
	  }
	  const certificate & common_channel_read_cert() const
	  {
		  return this->common_channel_read_cert_;
	  }

	  const asymmetric_private_key & common_channel_read_private_key() const {
		  return  this->common_channel_read_private_key_;
	  }

	  const certificate & common_channel_write_cert() const
	  {
		  return this->common_channel_write_cert_;
	  }

	  const asymmetric_private_key & common_channel_write_private_key() const {
		  return  this->common_channel_write_private_key_;
	  }

	  const std::list<certificate> & certificate_chain() const
	  {
		  return this->certificate_chain_;
	  }

	  const std::string & get_user_name() const { return this->user_name_; }

	  const_data_buffer pack(const std::string & user_password) const;
	  static user_invitation unpack(const const_data_buffer & data, const std::string & user_password);

  private:
	  guid user_id_;
	  std::string user_name_;
	  certificate user_certificate_;
	  asymmetric_private_key user_private_key_;

	  guid common_channel_id_;
	  certificate common_channel_read_cert_;
	  asymmetric_private_key common_channel_read_private_key_;
	  certificate common_channel_write_cert_;
	  asymmetric_private_key common_channel_write_private_key_;

	  std::list<certificate> certificate_chain_;

  };
}

#endif // __VDS_USER_MANAGER_USER_INVITATION_H_
