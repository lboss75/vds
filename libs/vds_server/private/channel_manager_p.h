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
      
      void add_member(
        const service_provider & sp,
        database_transaction & tr,
        const guid & channel_id,
        const guid & member_id);

      void remove_member(
        const service_provider & sp,
        database_transaction & tr,
        const guid & channel_id,
        const guid & member_id);
      
      std::list<guid> get_lead_certificates(
        const service_provider & sp,
        database_transaction & tr,
        const guid & channel_id);
      
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

      class channel_member_cert_table : public database_table
      {
      public:
        channel_member_cert_table()
        : database_table("channel_member_cert"),
          cert_id(this, "cert_id"),
          member_id(this, "member_id"),
          include_member(this, "include_member")
        {
        }

        database_column<guid> cert_id;
        database_column<guid> member_id;
        database_column<bool> include_member;
      };
  };
}

#endif // __VDS_SERVER_CHANNEL_MANAGERPRIVATE_H_
