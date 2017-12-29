
#include <db_model.h>
#include <transaction_log_record_dbo.h>
#include "stdafx.h"
#include "log_sync_service.h"
#include "private/log_sync_service_p.h"
#include "p2p_network.h"
#include "private/message_log_record_request.h"

vds::log_sync_service::log_sync_service() {

}

vds::log_sync_service::~log_sync_service() {

}

void vds::log_sync_service::start(const vds::service_provider &sp) {
  this->impl_.reset(new _log_sync_service());
  this->impl_->start(sp);
}

void vds::log_sync_service::stop(const vds::service_provider &sp) {
  this->impl_->stop(sp);
  this->impl_.reset();
}


/////////////////////////////////////////////////////////////////////
vds::_log_sync_service::_log_sync_service()
: update_timer_("Log Sync"), sycn_scheduled_(false){
}

vds::_log_sync_service::~_log_sync_service() {
}

void vds::_log_sync_service::start(const vds::service_provider &sp) {
  auto scope = sp.create_scope("Sync log");
  this->update_timer_.start(scope, std::chrono::seconds(1), [pthis = this->shared_from_this(), scope](){
    std::unique_lock<std::mutex> lock(pthis->state_mutex_);
    if(!pthis->sycn_scheduled_){
      pthis->sycn_scheduled_ = true;
      scope.get<db_model>()->async_transaction(scope, [scope, pthis](database_transaction & t){

        pthis->sync_process(scope, t);

        std::unique_lock<std::mutex> lock(pthis->state_mutex_);
        pthis->sycn_scheduled_ = false;
      }).execute([scope](std::shared_ptr<std::exception> & ex){
        if(ex){
          scope.get<logger>()->warning("LOGSYNC", scope, "Exception %s", ex->what());
        }
      });
    }
    return !scope.get_shutdown_event().is_shuting_down();
  });
}

void vds::_log_sync_service::sync_process(
    const vds::service_provider &sp,
    vds::database_transaction &t) {
  auto p2p = sp.get<p2p_network>();

  transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1
          .select(t1.id)
          .where(t1.state == (uint8_t)transaction_log_record_dbo::state_t::unknown));

  std::list<std::string> record_ids;
  while(st.execute()){
    auto record_id = t1.id.get(st);
    record_ids.push_back(record_id);
    if(record_ids.size() > 10) {
      this->request_unknown_records(sp, p2p, record_ids);
      record_ids.clear();
    }
  }
  if(!record_ids.empty()){
    this->request_unknown_records(sp, p2p, record_ids);
  }
}

void vds::_log_sync_service::request_unknown_records(
    const service_provider &sp,
    p2p_network *p2p,
    const std::list<std::string> &record_ids) {
  p2p->random_broadcast(sp, message_log_record_request(record_ids).serialize())
        .execute([sp](const std::shared_ptr<std::exception> &ex) {
          if (ex) {
            sp.get<logger>()->warning(
                "LOGSYNC",
                sp,
                "Exception at request records: %s",
                ex->what());
          }
        });
}

