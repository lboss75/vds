/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "transaction_log.h"
#include "private/transaction_log_p.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "db_model.h"
#include "transaction_block.h"
#include <set>
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "user_manager.h"
#include "member_user.h"
#include "cert_control.h"
#include "certificate_dbo.h"
#include "vds_exceptions.h"

void vds::transaction_log::save(
	const service_provider & sp,
	database_transaction & t,
	const const_data_buffer & block_id,
	const const_data_buffer & block_data)
{
	//Parse block
	guid common_read_cert_id;
	guid write_cert_id;
	const_data_buffer body;
	const_data_buffer key_crypted;
	const_data_buffer signature;

	binary_deserializer crypted(block_data);
	crypted
		>> common_read_cert_id
		>> write_cert_id
		>> body
		>> key_crypted;

	auto crypted_size = block_data.size() - crypted.size();
	crypted
		>> signature;
	
	//Validate
	bool is_validated = false;
	certificate_dbo t1;
	auto st = t.get_reader(t1.select(t1.cert).where(t1.id == write_cert_id));
	if (st.execute()) {
		auto cert = certificate::parse_der(t1.cert.get(st));
		if (!asymmetric_sign_verify::verify(hash::sha256(), cert.public_key(), signature, block_data.data(), crypted_size)) {
			throw vds_exceptions::signature_validate_error();
		}

		is_validated = true;
	}

	//
	auto user_mng = sp.get<user_manager>();

	asymmetric_private_key device_private_key;
	auto device_user = user_mng->get_current_device(sp, t, device_private_key);

	auto common_private_key = user_mng->get_private_key(
		t,
		common_read_cert_id,
		cert_control::get_id(device_user.user_certificate()),
		device_private_key);

	orm::transaction_log_record_dbo t2;
	st = t.get_reader(t2.select(t2.state).where(t2.id == base64::from_bytes(block_id)));

	if (!st.execute()) {
		//Not found
		t.execute(t2.insert(
			t2.id = base64::from_bytes(block_id),
			t2.data = block_data,
			t2.state = (uint8_t)orm::transaction_log_record_dbo::state_t::unknown));
	}
}
