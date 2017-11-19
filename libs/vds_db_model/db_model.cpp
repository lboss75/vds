#include "stdafx.h"
#include "db_model.h"

vds::async_task<> vds::db_model::async_transaction(const vds::service_provider &sp,
                                                   const std::function<void(vds::database_transaction &)> &handler) {
  return [this, sp, handler](const async_result<> & result){
    this->db_.async_transaction(sp, [handler, result](database_transaction & t)->bool{
      try {
        handler(t);
      }
      catch (const std::exception & ex){
        result.error(std::make_shared<std::runtime_error>(ex.what()));
        return true;
      }

      result.done();
      return true;
    });
  };
}
