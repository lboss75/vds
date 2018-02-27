/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include "transaction_log.h"
#include "private/transaction_log_p.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "db_model.h"
#include "transaction_block.h"
#include "transaction_log_unknown_record_dbo.h"
#include "transactions/channel_add_writer_transaction.h"
#include "transactions/device_user_add_transaction.h"
#include "transactions/channel_create_transaction.h"
#include "transactions/user_channel_create_transaction.h"
#include "run_configuration_dbo.h"
#include "vds_debug.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "user_manager.h"
#include "member_user.h"
#include "cert_control.h"
#include "vds_exceptions.h"
#include "encoding.h"
#include "cert_control.h"
#include "vds_debug.h"
#include "logger.h"
/*
void vds::transaction_log::save(
	const service_provider & sp,
	database_transaction & t,
	const const_data_buffer & block_id,
	const const_data_buffer & block_data)
{
	orm::transaction_log_record_dbo t2;
	auto st = t.get_reader(
      t2
          .select(t2.state)
          .where(t2.id == base64::from_bytes(block_id)));

	if (st.execute()) {
		return;//Already exists
	}

  std::list<const_data_buffer> followers;

  t.execute(t2.insert(
      t2.id = base64::from_bytes(block_id),
      t2.data = block_data,
      t2.state = (uint8_t)state));

  orm::transaction_log_unknown_record_dbo t4;
  for (auto &p : followers) {
    sp.get<logger>()->trace(
        ThisModule,
        sp,
        "Apply follower %s for %s",
        base64::from_bytes(p).c_str(),
        base64::from_bytes(block_id).c_str());

    t.execute(
        t4.delete_if(
            t4.id == base64::from_bytes(block_id)
            && t4.follower_id == base64::from_bytes(p)));
  }
}
*/