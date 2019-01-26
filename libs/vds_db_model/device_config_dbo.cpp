#include "stdafx.h"
#include "device_config_dbo.h"
#include "device_record_dbo.h"

vds::expected<std::list<vds::orm::device_config_dbo::device_info>> vds::orm::device_config_dbo::get_free_space(
  database_read_transaction & t,
  const const_data_buffer & node_id) {

  std::list<device_info> result;

  device_config_dbo t1;
  device_record_dbo t2;
  db_value<int64_t> used_size;
  GET_EXPECTED(st, t.get_reader(
    t1.select(
      t1.name,
      t1.node_id,
      t1.local_path,
      t1.reserved_size,
      db_sum(t2.data_size).as(used_size))
    .left_join(t2, t2.local_path == t1.local_path && t2.node_id == t1.node_id)
    .where(t1.node_id == node_id)
    .group_by(t1.name, t1.node_id, t1.local_path, t1.reserved_size)));

  for(;;){
    GET_EXPECTED(exec_result, st.execute());
    if (!exec_result) {
      break;
    }

    auto free_size = foldername(t1.local_path.get(st)).free_size();
    if(!free_size.has_value()) {
      continue;
    }

    const device_info record{
     t1.name.get(st),
     t1.local_path.get(st),
     t1.reserved_size.get(st),
     used_size.get(st),
     safe_cast<int64_t>(free_size.value())
    };

    result.push_back(record);
  }

  return result;
}
