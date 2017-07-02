#ifndef __VDS_PROTOCOLS_SERVER_TASK_MANAGER_P_H_
#define __VDS_PROTOCOLS_SERVER_TASK_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_task_manager.h"
#include "database_orm.h"

namespace vds {
 
  class _server_task_manager
  {
  public:
    _server_task_manager();
    ~_server_task_manager();
    
    void start(const service_provider & sp);
    void stop(const service_provider & sp);

  private:
    //Database
    class server_task_table : public database_table
    {
    public:
      server_task_table()
      : database_table("server_task"),
        task_id(this, "task_id"),
        task_title(this, "title"),
        task_status_text(this, "status_text")
      {
      }
      
      database_column<int> task_id;
      database_column<std::string> task_type;
      database_column<std::string> task_title;
      database_column<std::string> task_status_text;
    };
  };
}

#endif // __VDS_PROTOCOLS_SERVER_TASK_MANAGER_H_
