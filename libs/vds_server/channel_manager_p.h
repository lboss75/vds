#ifndef __VDS_SERVER_CHANNEL_MANAGERPRIVATE_H_
#define __VDS_SERVER_CHANNEL_MANAGERPRIVATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "database_orm.h"

namespace vds {
  class _channel_manager
  {
  public:
      _channel_manager();
      ~_channel_manager();

  private:
      class channel_table : public database_table
      {
      public:
        channel_table()
        : database_table("channel"),
          id(this, "id"),
          title(this, "title")
        {
        }
        
        database_column<guid> id;
        database_column<std::string> title;
      };
      
      class channel_member_table : public database_table
      {
      public:
        channel_member_table()
        : database_table("channel_member"),
          channel_id(this, "channel_id"),
          member_id(this, "member_id")
        {
        }
        
        database_column<guid> channel_id;
        database_column<guid> member_id;
      };
  };
}

/*

Channel: guid id, title

add member:
remove member:

crypt data:
  if(added members != 0 || removed members != 0){
    new channel key
    to old members -> encrypt new key by old key,
    to new member -> encrypt new key by open key from member's certificate
  }
  
  create file key
  key_id,
  crypt file_key by the channel key
  
  
*/


#endif // __VDS_SERVER_CHANNEL_MANAGERPRIVATE_H_
