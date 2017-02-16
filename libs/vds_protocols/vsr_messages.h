#ifndef __VDS_PROTOCOLS_VSR_MESSAGES_H_
#define __VDS_PROTOCOLS_VSR_MESSAGES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class json_writer;
  class json_object;
  
  struct vsr_message
  {
  };

  struct vsr_new_client_message
  {
    static const char message_type[];

    void serialize(json_writer & writer) const;

    static vsr_new_client_message deserialize(const json_object * task_data);
  };

  struct vsr_new_client_message_complete
  {
    size_t client_id;
    size_t current_primary_view;
    size_t server_count;
  };

}


#endif // __VDS_PROTOCOLS_VSR_MESSAGES_H_
