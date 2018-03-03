
#include "stdafx.h"
#include "log_sync_service.h"
#include "private/log_sync_service_p.h"
#include "p2p_network.h"
#include "db_model.h"
#include "transaction_log_record_dbo.h"
#include "chunk_manager.h"
#include "transaction_block.h"
#include "messages/channel_log_record.h"
#include "messages/channel_log_state.h"
#include "messages/channel_log_request.h"
#include "transaction_log.h"
#include "transaction_log_unknown_record_dbo.h"

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

vds::async_task<> vds::log_sync_service::prepare_to_stop(const vds::service_provider &sp) {
  return this->impl_->prepare_to_stop(sp);
}

void vds::log_sync_service::get_statistic(database_transaction & t, vds::sync_statistic & result) {
  this->impl_->get_statistic(t, result);
}

void vds::log_sync_service::apply(
    const vds::service_provider &sp,
    const vds::guid &partner_id,
    const vds::p2p_messages::channel_log_state &message) {
  this->impl_->apply(sp, partner_id, message);
}

void vds::log_sync_service::apply(
	const vds::service_provider &sp,
	const vds::guid &partner_id,
	const vds::p2p_messages::channel_log_request &message) {
	this->impl_->apply(sp, partner_id, message);
}

void vds::log_sync_service::apply(
	const vds::service_provider &sp,
	const vds::guid &partner_id,
	const vds::p2p_messages::channel_log_record &message) {
	this->impl_->apply(sp, partner_id, message);
}

/////////////////////////////////////////////////////////////////////
vds::_log_sync_service::_log_sync_service()
: update_timer_("Log Sync"), sycn_scheduled_(false){
}

vds::_log_sync_service::~_log_sync_service() {
}

void vds::_log_sync_service::start(const vds::service_provider &sp) {
  auto scope = sp.create_scope("Sync log");
  mt_service::enable_async(scope);
  this->update_timer_.start(scope, std::chrono::seconds(1), [pthis = this->shared_from_this(), scope](){
    std::unique_lock<std::mutex> lock(pthis->state_mutex_);
    if(!pthis->sycn_scheduled_){
      pthis->sycn_scheduled_ = true;
      scope.get<db_model>()->async_transaction(scope, [scope, pthis](database_transaction & t){

        pthis->sync_process(scope, t);

        std::unique_lock<std::mutex> lock(pthis->state_mutex_);
        pthis->sycn_scheduled_ = false;
      }).execute([scope](const std::shared_ptr<std::exception> & ex){
        if(ex){
          scope.get<logger>()->warning(ThisModule, scope, "Exception %s", ex->what());
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
  this->send_current_state(sp, t, p2p);
  this->ask_unknown_records(sp, t, p2p);
  this->ask_unknown_certificates(sp, t, p2p);
}

void vds::_log_sync_service::ask_unknown_records(const vds::service_provider &sp, vds::database_transaction &t,
                                            vds::p2p_network *p2p) const {
  std::map<guid, std::list<const_data_buffer>> record_ids;
  orm::transaction_log_unknown_record_dbo t1;
  auto st = t.get_reader(t1.select(t1.id, t1.channel_id));

  while(st.execute()){
    record_ids[t1.channel_id.get(st)].push_back(base64::to_bytes(t1.id.get(st)));
  }

  for(auto p : record_ids){

    std::string log_message;
    for(const auto & r : p.second) {
      log_message += base64::from_bytes(r);
      log_message += ' ';
    }

    sp.get<logger>()->trace(
      ThisModule,
      sp,
      "Query log records %s of channel %s",
        log_message.c_str(),
        p.first.str().c_str());

    p2p->send_tentatively(
        sp,
        p.first,
        p2p_messages::channel_log_request(
            p.first,
            p.second,
            p2p->current_node_id()).serialize(),
        1024);
  }
}

void vds::_log_sync_service::ask_unknown_certificates(const service_provider& sp, const database_transaction& t,
  p2p_network* p2p) {
}

void vds::_log_sync_service::send_current_state(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    vds::p2p_network *p2p) const {
  std::map<guid, std::list<const_data_buffer>> state;

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1
          .select(t1.id, t1.channel_id)
          .where(t1.state == (int)orm::transaction_log_record_dbo::state_t::leaf));
  while(st.execute()){
    auto id = t1.id.get(st);
    auto channel_id = t1.channel_id.get(st);

    state[channel_id].push_back(base64::to_bytes(id));
  }

  for(auto p : state){

    std::string log_message;
    for (const auto & r : p.second) {
      log_message += base64::from_bytes(r);
      log_message += ' ';
    }

    sp.get<logger>()->trace(
      ThisModule,
      sp,
      "Channel %s state is %s",
      p.first.str().c_str(),
      log_message.c_str());

    p2p->send_tentatively(
        sp,
        p.first,
        p2p_messages::channel_log_state(
            p.first,
            p.second,
            p2p->current_node_id()).serialize(),
        1024);
  }
}

void vds::_log_sync_service::get_statistic(
    database_transaction & t,
    vds::sync_statistic &result) {
  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1.select(t1.id, t1.channel_id)
          .where(t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
  while(st.execute()){
    result.leafs_[t1.channel_id.get(st)].push_back(t1.id.get(st));
  }
}

void vds::_log_sync_service::stop(const vds::service_provider &sp) {
  this->update_timer_.stop(sp);
}

vds::async_task<> vds::_log_sync_service::prepare_to_stop(const vds::service_provider &sp) {
  return vds::async_task<>::empty();
}

void vds::_log_sync_service::apply(const vds::service_provider &sp, const vds::guid &partner_id,
                              const vds::p2p_messages::channel_log_state &message) {
  sp.get<db_model>()->async_transaction(
      sp,
      [pthis = this->shared_from_this(), sp, partner_id, message](database_transaction & t) -> bool{
        orm::transaction_log_record_dbo t1;

        std::list<const_data_buffer> requests;
        for(auto & p : message.leafs()){
          auto st = t.get_reader(
              t1.select(t1.state)
                  .where(t1.id == base64::from_bytes(p)));
          if(!st.execute()){
            //Not found
            requests.push_back(p);
          }
        }

        if(!requests.empty()){
          std::string log_message;
          for (const auto & r : requests) {
            log_message += base64::from_bytes(r);
            log_message += ' ';
          }

          sp.get<logger>()->trace(
            ThisModule,
            sp,
            "Query log records %s of channel %s",
            log_message.c_str(),
            message.channel_id().str().c_str());

          auto p2p = sp.get<p2p_network>();
          p2p->send(
              sp,
              message.source_node(),
              p2p_messages::channel_log_request(
                  message.channel_id(),
                  requests,
                  p2p->current_node_id()).serialize());
        }

        return true;
  }).execute([sp, partner_id](const std::shared_ptr<std::exception> & ex){
    if(ex){
      //sp.get<p2p_network>()->close_session(sp, partner_id, ex);
    }
  });

}

void vds::_log_sync_service::apply(const vds::service_provider &sp, const vds::guid &partner_id,
	const vds::p2p_messages::channel_log_request &message) {
	sp.get<db_model>()->async_transaction(
		sp,
		[pthis = this->shared_from_this(), sp, partner_id, message](database_transaction & t) -> bool{

		auto p2p = sp.get<p2p_network>();

		orm::transaction_log_record_dbo t1;
		std::list<const_data_buffer> requests;
		for (auto & p : message.requests()) {
			auto st = t.get_reader(t1.select(t1.channel_id, t1.data).where(t1.id == base64::from_bytes(p)));
			if (st.execute()) {

        sp.get<logger>()->trace(
          ThisModule,
          sp,
          "Provide log record %s of channel %s",
          base64::from_bytes(p).c_str(),
          t1.channel_id.get(st).str().c_str());

        p2p->send(
					sp,
					message.source_node(),
					p2p_messages::channel_log_record(
              t1.channel_id.get(st),
              p,
              t1.data.get(st)).serialize());
			}
		}

		return true;
	}).execute([sp, partner_id](const std::shared_ptr<std::exception> & ex) {
		if (ex) {
//			sp.get<p2p_network>()->close_session(sp, partner_id, ex);
		}
	});

}

void vds::_log_sync_service::apply(const vds::service_provider &sp, const vds::guid &partner_id,
	const vds::p2p_messages::channel_log_record &message) {
	sp.get<db_model>()->async_transaction(
		sp,
		[pthis = this->shared_from_this(), sp, partner_id, message](database_transaction & t) -> bool{

    sp.get<logger>()->trace(
      ThisModule,
      sp,
      "Save log record %s of channel %s",
      base64::from_bytes(message.record_id()).c_str(),
      message.channel_id().str().c_str());

    transaction_log::save(sp, t, message.channel_id(), message.record_id(), message.data());

		return true;
	}).execute([sp, partner_id](const std::shared_ptr<std::exception> & ex) {
		if (ex) {
			//sp.get<p2p_network>()->close_session(sp, partner_id, ex);
		}
	});

}
