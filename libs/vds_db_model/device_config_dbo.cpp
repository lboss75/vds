#include "stdafx.h"
#include "device_config_dbo.h"
#include "device_record_dbo.h"

std::list<vds::orm::device_config_dbo::device_info> vds::orm::device_config_dbo::get_free_space(
  database_read_transaction & t,
  const const_data_buffer & node_id) {

  std::list<device_info> result;

  device_config_dbo t1;
  device_record_dbo t2;
  db_value<uint64_t> used_size;
  auto st = t.get_reader(
    t1.select(
      t1.name,
      t1.node_id,
      t1.local_path,
      t1.reserved_size,
      db_sum(t2.data_size).as(used_size))
    .left_join(t2, t2.local_path == t1.local_path && t2.node_id == t1.node_id)
    .where(t1.node_id == base64::from_bytes(node_id))
    .group_by(t1.name, t1.node_id, t1.local_path, t1.reserved_size));
  while (st.execute()) {
    try {
      const device_info record{
       t1.name.get(st),
       t1.local_path.get(st),
       t1.reserved_size.get(st),
       used_size.get(st),
       foldername(t1.local_path.get(st)).free_size()
      };

      result.push_back(record);
    }
    catch (const std::system_error & ex) {
    }
  }

  return result;
}
