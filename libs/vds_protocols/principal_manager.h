#ifndef __VDS_PROTOCOLS_PRINCIPAL_MANAGER_H_
#define __VDS_PROTOCOLS_PRINCIPAL_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include "principal_record.h"
#include "log_records.h"

namespace vds {
  class database_transaction;
  class _principal_manager;
  
  class principal_manager
  {
  public:
    principal_manager();
    ~principal_manager();

    void lock();
    void unlock();

    bool save_record(
      const service_provider & sp,
      database_transaction & tr,
      const principal_log_record & record,
      const const_data_buffer & signature);

    std::unique_ptr<principal_record> find_principal(
      const service_provider & sp,
      database_transaction & tr,
      const guid & object_name);

    std::unique_ptr<principal_record> find_user_principal(
      const service_provider & sp,
      database_transaction & tr,
      const std::string & object_name);

    size_t get_current_state(
      const service_provider & sp,
      database_transaction & tr,
      std::list<guid> & active_records);


    void get_principal_log(
      const service_provider & sp,
      database_transaction & tr,
      const guid & principal_id,
      size_t last_order_num,
      size_t & result_last_order_num,
      std::list<principal_log_record> & records);

    _principal_manager * operator -> () { return this->impl_; }
  private:
    _principal_manager * const impl_;
  };
}

#endif // __VDS_PROTOCOLS_PRINCIPAL_MANAGER_H_
