/*
#include <certificate_unknown_dbo.h>
#include <user_manager.h>
#include "stdafx.h"
#include "log_sync_service.h"
#include "private/log_sync_service_p.h"
#include "db_model.h"
#include "transaction_log_record_dbo.h"
#include "chunk_manager.h"
#include "transaction_block.h"
#include "messages/channel_log_record.h"
#include "messages/channel_log_state.h"
#include "messages/channel_log_request.h"
#include "transaction_log.h"
#include "transaction_log_unknown_record_dbo.h"
#include "vds_debug.h"
#include "chunk_replicator.h"

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

  sync_statistic stat;
  this->get_statistic(t, stat);
  sp.get<logger>()->trace(ThisModule, sp, "Statistic: %s", stat.str().c_str());
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

void vds::_log_sync_service::ask_unknown_certificates(
    const service_provider& sp,
    database_transaction& t,
    p2p_network* p2p) {
  std::set<guid> unknown_certs;
  orm::certificate_unknown_dbo t1;
  auto st = t.get_reader(t1.select(t1.id));
  while(st.execute()){
    unknown_certs.emplace(t1.id.get(st));
  }

  if(unknown_certs.empty()){
    return;
  }

  const auto chunk_rpt = sp.get<chunk_replicator>();
  auto user_mng = sp.get<user_manager>();
  for(auto p : unknown_certs) {
    const auto cert_data = chunk_rpt->get_object(sp, t, p);
    if(!cert_data){
      continue;
    }

    auto cert = certificate::parse_der(cert_data);
    user_mng->save_certificate(sp, t, cert);
  }

  this->try_to_validate_records(sp, t);
}

void vds::_log_sync_service::send_current_state(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    vds::p2p_network *p2p) {
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

    std::shared_lock<std::shared_mutex> lock(this->channel_subscribers_mutex_);
    auto p_channel = this->channel_subscribers_.find(p.first);
    if (this->channel_subscribers_.end() != p_channel) {
      for (const auto & node : p_channel->second) {
        p2p->send(
          sp,
          node,
          p2p_messages::channel_log_state(
            p.first,
            p.second,
            p2p->current_node_id()).serialize());
      }
    }

  }
}

void vds::_log_sync_service::get_statistic(
  database_transaction & t,
  vds::sync_statistic &result) {
  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1.select(t1.id, t1.channel_id)
    .where(t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
  while (st.execute()) {
    result.leafs_[t1.channel_id.get(st)].push_back(t1.id.get(st));
  }

  std::map<std::string, std::shared_ptr<sync_statistic::record_info>> index;
  std::map<std::string, std::shared_ptr<sync_statistic::record_info>> roots;
  st = t.get_reader(t1.select(t1.id, t1.state, t1.data));
  while (st.execute()) {
    guid channel_id;
    uint64_t order_no;
    guid read_cert_id;
    guid write_cert_id;
    std::set<const_data_buffer> ancestors;
    const_data_buffer crypted_data;
    const_data_buffer crypted_key;
    const_data_buffer signature;

    transactions::transaction_block::parse_block(
      t1.data.get(st),
      channel_id,
      order_no,
      read_cert_id,
      write_cert_id,
      ancestors,
      crypted_data,
      crypted_key,
      signature);

    std::shared_ptr<sync_statistic::record_info> node;
    auto id = t1.id.get(st);
    auto p = index.find(id);
    if (index.end() == p) {
      node = std::make_shared<sync_statistic::record_info>();
      index[id] = node;
      roots[id] = node;
    }
    else {
      node = p->second;
    }

    node->name_ = id;
    switch ((orm::transaction_log_record_dbo::state_t)t1.state.get(st)) {
    case orm::transaction_log_record_dbo::state_t::leaf:
      node->state_ = "leaf";
      break;
    case orm::transaction_log_record_dbo::state_t::stored:
      node->state_ = "stored";
      break;
    case orm::transaction_log_record_dbo::state_t::validated:
      node->state_ = "validated";
      break;
    default:
      node->state_ = "*** invalid ***";
      break;
    }
    for (const auto & ancestor : ancestors) {
      std::shared_ptr<sync_statistic::record_info> child_node;
      const auto child_p = index.find(base64::from_bytes(ancestor));
      if (index.end() == child_p) {
        child_node = std::make_shared<sync_statistic::record_info>();
        index[base64::from_bytes(ancestor)] = child_node;
      }
      else {
        child_node = child_p->second;
        const auto root_p = roots.find(base64::from_bytes(ancestor));
        if (roots.end() != root_p) {
          roots.erase(root_p);
        }
      }

      node->children_.push_back(child_node);
    }
  }

  orm::transaction_log_unknown_record_dbo t2;
  st = t.get_reader(t2.select(t2.id, t2.follower_id));
  while (st.execute()) {
    std::shared_ptr<sync_statistic::record_info> node;
    auto id = t2.id.get(st);
    auto p = index.find(id);
    if (index.end() == p) {
      node = std::make_shared<sync_statistic::record_info>();
      index[id] = node;
      roots[id] = node;
    }
    else {
      node = p->second;
    }

    node->name_ = id;
    vds_assert(node->state_.empty() || node->state_ == "unknown");
    node->state_ = "unknown";

    const auto child_p = index.find(t2.follower_id.get(st));
    vds_assert(index.end() != child_p);
    const auto & child_node = child_p->second;
    const auto root_p = roots.find(t2.follower_id.get(st));
    if (roots.end() != root_p) {
      roots.erase(root_p);
    }

    node->children_.push_back(child_node);
  }

  for(auto & p : roots) {
    result.roots_.push_back(p.second);
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

    pthis->add_subscriber(sp, message.channel_id(), message.source_node());

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
        else {
          std::string log_message;
          for (const auto & r : message.leafs()) {
            log_message += base64::from_bytes(r);
            log_message += ' ';
          }

          sp.get<logger>()->trace(
            ThisModule,
            sp,
            "log records %s of channel %s already exists",
            log_message.c_str(),
            message.channel_id().str().c_str());
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

    pthis->send_to_subscribles(sp, t, message.channel_id());

    return true;
  }).execute([sp, partner_id](const std::shared_ptr<std::exception> & ex) {
		if (ex) {
			//sp.get<p2p_network>()->close_session(sp, partner_id, ex);
		}
	});
}

void vds::_log_sync_service::add_subscriber(const service_provider& sp, const guid& channel_id,
  const guid& source_node_id) {
  std::unique_lock<std::shared_mutex> lock(this->channel_subscribers_mutex_);
  const auto p = this->channel_subscribers_[channel_id].find(source_node_id);
  if(this->channel_subscribers_[channel_id].end() == p) {
    this->channel_subscribers_[channel_id].emplace(source_node_id);
  }
}

void vds::_log_sync_service::send_to_subscribles(
  const service_provider& sp,
  database_transaction& t,
  const guid& channel_id) {
  std::list<const_data_buffer> state;
  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1
    .select(t1.id)
    .where(t1.channel_id == channel_id
      && t1.state == (int)orm::transaction_log_record_dbo::state_t::leaf));

  while (st.execute()) {
    auto id = t1.id.get(st);

    state.push_back(base64::to_bytes(id));
  }

  std::shared_lock<std::shared_mutex> lock(this->channel_subscribers_mutex_);
  auto p_channel = this->channel_subscribers_.find(channel_id);
  if (this->channel_subscribers_.end() != p_channel) {
    auto p2p = sp.get<p2p_network>();
    for (auto node : p_channel->second) {
      p2p->send(
        sp,
        node,
        p2p_messages::channel_log_state(
          channel_id,
          state,
          p2p->current_node_id()).serialize());
    }
  }
}

struct processed_record_info {
  std::set<vds::const_data_buffer> ancestors_;

  processed_record_info(){
  }

  processed_record_info(
      const std::set<vds::const_data_buffer> & ancestors)
  : ancestors_(ancestors){
  }
};

void vds::_log_sync_service::try_to_validate_records(
    const vds::service_provider &sp,
    vds::database_transaction &t) {

  std::map<std::string, processed_record_info> processed_records;
  std::list<std::string> invalid_records;

  auto user_mng = sp.get<user_manager>();
  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1
          .select(t1.id, t1.data)
          .where(t1.state == (int)orm::transaction_log_record_dbo::state_t::stored));

  while(st.execute()) {
    auto block_data = t1.data.get(st);

    guid block_channel_id;
    uint64_t order_no;
    guid read_cert_id;
    guid write_cert_id;
    std::set<const_data_buffer> ancestors;
    const_data_buffer crypted_data;
    const_data_buffer crypted_key;
    const_data_buffer signature;
    transactions::transaction_block::parse_block(
        block_data,
        block_channel_id,
        order_no,
        read_cert_id,
        write_cert_id,
        ancestors,
        crypted_data,
        crypted_key,
        signature);

    auto write_cert = user_mng->get_channel_write_cert(sp, block_channel_id, write_cert_id);
    if (!write_cert) {
      continue;
    } else {
      if (!transactions::transaction_block::validate_block(write_cert, block_channel_id, order_no, read_cert_id,
                                                           write_cert_id, ancestors,
                                                           crypted_data, crypted_key, signature)) {
        invalid_records.push_back(t1.id.get(st));
      } else {
        processed_records[t1.id.get(st)] = processed_record_info(ancestors);
      }
    }
  }

  if(!processed_records.empty()) {
    for(const auto & p : processed_records) {
      if(p.second.ancestors_.empty()) {
        t.execute(
            t1.update(
                    t1.state = (int) orm::transaction_log_record_dbo::state_t::leaf)
                .where(t1.id == p.first));
      }
      else {
        t.execute(
            t1.update(
                    t1.state = (int) orm::transaction_log_record_dbo::state_t::validated)
                .where(t1.id == p.first));
      }
    }
  }

  if(!invalid_records.empty()) {
    t.execute(t1.delete_if(db_in_values(t1.id, invalid_records)));
  }
}
*/