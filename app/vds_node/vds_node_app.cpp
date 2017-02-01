#include "stdafx.h"
#include "vds_node_app.h"

vds::vds_node_app::vds_node_app()
:
add_storage_cmd_set_(
  "Add new storage",
  "Add new storage to the network",
  "add",
  "storage"
),
remove_storage_cmd_set_(
  "Remove storage",
  "Remove storage to the network",
  "remove",
  "storage"
),
list_storage_cmd_set_(
  "List storage",
  "List storage to the network",
  "list",
  "storage"
),
storage_path_(
  "s", "storage",
  "Storage", "Path to the storage"
)
{

}

void vds::vds_node_app::main(
  const vds::service_provider& sp)
{

}

void vds::vds_node_app::register_command_line(vds::vds_command_line& cmd_line)
{
  cmd_line.add_command_set(this->add_storage_cmd_set_);
  this->add_storage_cmd_set_.required(this->storage_path_);

}
