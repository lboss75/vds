#include "stdafx.h"
#include "current_config_dbo.h"
#include "device_record_dbo.h"

vds::expected<vds::orm::current_config_dbo::device_info> vds::orm::current_config_dbo::get_free_space(
  database_read_transaction & t) {

  device_info result;

  current_config_dbo t1;
  device_record_dbo t2;
  db_value<int64_t> used_size;
  GET_EXPECTED(st, t.get_reader(
    t1.select(
      t1.node_id,
      t1.local_path,
      t1.reserved_size,
      db_sum(t2.data_size).as(used_size))
    .left_join(t2, t2.node_id == t1.node_id)));

  GET_EXPECTED(exec_result, st.execute());
  if (!exec_result) {
    return result;
  }

  auto free_size = foldername(t1.local_path.get(st)).free_size();
  if(!free_size.has_value()) {
    return result;
  }

  result.local_path = t1.local_path.get(st);
  result.reserved_size = t1.reserved_size.get(st);
  result.used_size = used_size.get(st);
  result.free_size = safe_cast<int64_t>(free_size.value());

  return result;
}
