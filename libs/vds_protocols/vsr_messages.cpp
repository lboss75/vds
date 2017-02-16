/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vsr_messages.h"

const char vds::vsr_new_client_message::message_type[] = "vsr_new_client";

void vds::vsr_new_client_message::serialize(json_writer & writer) const
{
  writer.start_object();
  writer.write_property("$type", message_type);
  writer.end_object();
}

vds::vsr_new_client_message vds::vsr_new_client_message::deserialize(const json_object * task_data)
{
  return vsr_new_client_message();
}
