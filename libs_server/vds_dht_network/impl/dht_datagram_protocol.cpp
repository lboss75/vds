/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "dht_datagram_protocol.h"
#include "iudp_transport.h"

vds::dht::network::dht_datagram_protocol::dht_datagram_protocol(const service_provider* sp, const network_address& address, const const_data_buffer& this_node_id, asymmetric_public_key partner_node_key, const const_data_buffer& partner_node_id, const const_data_buffer& session_key) noexcept
  : sp_(sp),
  failed_state_(false),
  check_mtu_(0),
  last_sent_(0),
  address_(address),
  this_node_id_(this_node_id),
  partner_node_key_(std::move(partner_node_key)),
  partner_node_id_(partner_node_id),
  session_key_(session_key),
  mtu_(MIN_MTU),
  input_mtu_(0),
  last_output_index_(0),
  last_input_index_(0),
  expected_index_(0),
  last_processed_(std::chrono::steady_clock::now()) {
}

vds::dht::network::dht_datagram_protocol::~dht_datagram_protocol()
{
}

void vds::dht::network::dht_datagram_protocol::set_mtu(uint16_t value) {
  this->mtu_ = value;
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::send_message(
  const std::shared_ptr<iudp_transport>& s,
  uint8_t message_type,
  const const_data_buffer& target_node,
  expected<const_data_buffer>&& message) {
  CHECK_EXPECTED_ERROR(message);
  return this->send_message(s, message_type, target_node, message.value());
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::send_message(
  const std::shared_ptr<iudp_transport>& s,
  uint8_t message_type,
  const const_data_buffer& target_node,
  const const_data_buffer& message) {
  vds_assert(message.size() <= 0xFFFFFFFF);
  vds_assert(target_node != this->this_node_id_);
  vds_assert(0 == (message_type & static_cast<uint8_t>(protocol_message_type_t::SpecialCommand)));

  logger::get(this->sp_)->trace(
    "dht_session",
    "send %d from this node %s to %s",
    message_type,
    base64::from_bytes(this->this_node_id_).c_str(),
    base64::from_bytes(target_node).c_str());

  std::unique_lock<std::mutex> lock(this->traffic_mutex_);
  this->traffic_[base64::from_bytes(this->this_node_id_)][base64::from_bytes(target_node)][message_type].sent_count_++;
  this->traffic_[base64::from_bytes(this->this_node_id_)][base64::from_bytes(target_node)][message_type].sent_ += message.size();

  if (this->output_messages_.size() > 10000) {
    std::cout << "Big Queue" << std::endl;
    for (auto& item : this->traffic_[base64::from_bytes(this->this_node_id_)]) {
      for (auto& sub_item : item.second) {
        if (sub_item.second.sent_count_ > 0) {
          std::cout << "[" << int(sub_item.first) << "]" << sub_item.second.sent_count_ << " to " << item.first << std::endl;
        }
      }
    }
    std::cout << "-----" << std::endl;
  }
  lock.unlock();

  return this->send_message_async(
    s,
    message_type,
    target_node,
    std::vector<const_data_buffer>(),
    message);
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::proxy_message(
  std::shared_ptr<iudp_transport> s,
  uint8_t message_type,
  const_data_buffer target_node,
  std::vector<const_data_buffer> hops,
  const_data_buffer message) {
  vds_assert(message.size() <= 0xFFFFFFFF);
  vds_assert(target_node != this->this_node_id_);
  vds_assert(hops[0] != this->partner_node_id_);
  vds_assert(hops.size() < 0xFF);
  vds_assert(this->mtu_ > 1 + 4 + SIZE_SIZE + 32 + 1 + hops.size() * 32 + 32);
  vds_assert(0 == (message_type & static_cast<uint8_t>(protocol_message_type_t::SpecialCommand)));

  logger::get(this->sp_)->trace(
    "dht_session",
    "send %d from %s to %s",
    message_type,
    base64::from_bytes(hops[0]).c_str(),
    base64::from_bytes(target_node).c_str());

  std::unique_lock<std::mutex> lock(this->traffic_mutex_);
  this->traffic_[base64::from_bytes(hops[0])][base64::from_bytes(target_node)][message_type].sent_count_++;
  this->traffic_[base64::from_bytes(hops[0])][base64::from_bytes(target_node)][message_type].sent_ += message.size();
  lock.unlock();

  return this->send_message_async(
    s,
    message_type,
    target_node,
    std::move(hops),
    std::move(message));
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::process_datagram(const std::shared_ptr<iudp_transport>& s, const_data_buffer datagram) {

  if (this->failed_state_) {
    co_return make_unexpected<std::runtime_error>("failed state");
  }

  if (this->last_sent_ > SEND_TIMEOUT) {
    co_return make_unexpected<std::runtime_error>("Send timeout");
  }

  if (this->input_mtu_ < datagram.size()) {
    this->input_mtu_ = safe_cast<decltype(this->input_mtu_)>(datagram.size());
  }

  if (protocol_message_type_t::MTUTest == static_cast<protocol_message_type_t>(*datagram.data())) {
    co_return vds::expected<void>();
  }

  if (protocol_message_type_t::Acknowledgment == static_cast<protocol_message_type_t>(*datagram.data())) {
    co_return co_await this->process_acknowledgment(s, datagram);
  }

  if (datagram.size() < 33) {
    co_return vds::make_unexpected<std::runtime_error>("Invalid data");
  }

  if (!hmac::verify(
    this->session_key_,
    hash::sha256(),
    datagram.data(), datagram.size() - 32,
    datagram.data() + datagram.size() - 32, 32)) {

    co_return vds::make_unexpected<std::runtime_error>("Invalid signature");
  }

  switch (static_cast<protocol_message_type_t>((uint8_t)protocol_message_type_t::SpecialCommand & *datagram.data())) {
  case protocol_message_type_t::SingleData:
  case protocol_message_type_t::RouteSingleData:
  case protocol_message_type_t::ProxySingleData:
  case protocol_message_type_t::Data:
  case protocol_message_type_t::RouteData:
  case protocol_message_type_t::ProxyData: {
    break;
  }
  default: {
    if (datagram.data()[0] != (uint8_t)protocol_message_type_t::ContinueData) {
      co_return vds::make_unexpected<std::runtime_error>("Invalid data");
    }
    break;
  }
  }

  const auto index =
    (datagram.data()[1] << 24)
    | (datagram.data()[1 + 1] << 16)
    | (datagram.data()[1 + 2] << 8)
    | (datagram.data()[1 + 3]);

  std::unique_lock<std::mutex> lock(this->input_mutex_);

  if (this->last_processed_ < std::chrono::steady_clock::now() - std::chrono::minutes(10)) {
    co_return vds::make_unexpected<std::runtime_error>("Process timeout");
  }

  bool is_new = (this->input_messages_.end() == this->input_messages_.find(index));
  if (is_new) {
    this->sp_->template get<logger>()->trace(
      ThisModule,
      "%s->%s[%d] got %d(%d,%d,%d,%d)",
      base64::from_bytes(this->partner_node_id_).c_str(),
      base64::from_bytes(this->this_node_id_).c_str(),
      index,
      (int)(datagram.data()[0]),
      datagram.size(),
      this->input_messages_.size(),
      this->last_input_index_,
      this->expected_index_);
    this->input_messages_[index] = std::move(datagram);
  }
  lock.unlock();

  if (is_new) {
    CHECK_EXPECTED_ASYNC(co_await this->continue_process_messages(s));

    if (index > this->expected_index_ + 32) {
      mt_service::async(this->sp_, [pthis = this->shared_from_this(), s](){
        pthis->send_acknowledgment(s).then([](expected<void>) {});
      });
    }
  }
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::on_timer(const std::shared_ptr<iudp_transport>& s) {
  this->check_mtu_++;
  this->last_sent_++;
  if (this->check_mtu_ < CHECK_MTU_TIMEOUT) {
    if (this->mtu_ < 0xFFFF - 256) {
      std::unique_lock<std::mutex> lock(this->output_mutex_);
      auto mtu = this->mtu_;
      lock.unlock();

      resizable_data_buffer out_message;
      CHECK_EXPECTED(out_message.resize_data(mtu + 256));
      CHECK_EXPECTED(out_message.add((uint8_t)protocol_message_type_t::MTUTest));
      out_message.apply_size(mtu + 255);

      (void)co_await s->write_async(udp_datagram(this->address_, out_message.move_data()));
    }
  }

  CHECK_EXPECTED_ASYNC(co_await this->send_acknowledgment(s));

  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::send_acknowledgment(const std::shared_ptr<iudp_transport>& s) {

  resizable_data_buffer out_message;
  CHECK_EXPECTED(out_message.add((uint8_t)protocol_message_type_t::Acknowledgment));

  std::unique_lock<std::mutex> lock(this->input_mutex_);
  CHECK_EXPECTED(out_message.add((uint8_t)((this->last_input_index_) >> 24)));//1
  CHECK_EXPECTED(out_message.add((uint8_t)((this->last_input_index_) >> 16)));//1
  CHECK_EXPECTED(out_message.add((uint8_t)((this->last_input_index_) >> 8)));//1
  CHECK_EXPECTED(out_message.add((uint8_t)((this->last_input_index_) & 0xFF)));//1

  this->sp_->template get<logger>()->trace(
    ThisModule,
    "%s->%s ask %d",
    base64::from_bytes(this->this_node_id_).c_str(),
    base64::from_bytes(this->partner_node_id_).c_str(),
    this->last_input_index_);

  uint32_t mask = 0;
  for (int i = 32; i > 0; --i) {
    mask <<= 1;

    if (this->input_messages_.end() == this->input_messages_.find(this->last_input_index_ + i)) {
      this->sp_->template get<logger>()->trace(
        ThisModule,
        "%s->%s mask %d",
        base64::from_bytes(this->this_node_id_).c_str(),
        base64::from_bytes(this->partner_node_id_).c_str(),
        this->last_input_index_ + i);

      mask |= 1;
    }
  }
  lock.unlock();

  CHECK_EXPECTED(out_message.add((uint8_t)((mask) >> 24)));//1
  CHECK_EXPECTED(out_message.add((uint8_t)((mask) >> 16)));//1
  CHECK_EXPECTED(out_message.add((uint8_t)((mask) >> 8)));//1
  CHECK_EXPECTED(out_message.add((uint8_t)((mask) & 0xFF)));//1

  CHECK_EXPECTED(out_message.add((uint8_t)((this->input_mtu_) >> 8)));//1
  CHECK_EXPECTED(out_message.add((uint8_t)((this->input_mtu_) & 0xFF)));//1

  return s->write_async(udp_datagram(this->address_, out_message.move_data()));
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::send_message_async(
  std::shared_ptr<iudp_transport> s,
  uint8_t message_type,
  const_data_buffer target_node,
  std::vector<const_data_buffer> hops,
  const_data_buffer message) {

  GET_EXPECTED_ASYNC(indexes, this->prepare_to_send(
    message_type,
    std::move(target_node),
    std::move(hops),
    std::move(message)));

  for (uint32_t start_index = std::get<0>(indexes); start_index < std::get<1>(indexes); ++start_index) {
    std::unique_lock<std::mutex> lock(this->output_mutex_);
    auto p = this->output_messages_.find(start_index);
    if (this->output_messages_.end() == p) {
      continue;
    }

    udp_datagram datagram(this->address_, p->second);
    lock.unlock();

    CHECK_EXPECTED_ASYNC(co_await s->write_async(datagram));
  }
  co_return expected<void>();
}

vds::expected<std::tuple<uint32_t, uint32_t>> vds::dht::network::dht_datagram_protocol::prepare_to_send(
  uint8_t message_type,
  const_data_buffer target_node,
  std::vector<const_data_buffer> hops,
  const_data_buffer message) {

  std::unique_lock<std::mutex> lock(this->output_mutex_);
  uint32_t start_index = this->last_output_index_;

  size_t total_size = 1 + 4 + 32 + message.size();
  if (hops.empty()) {
    if (this->partner_node_id_ != target_node) {
      total_size += 32;
    }
  }
  else {
    total_size += 32 + 1 + hops.size() * 32;
  }

  resizable_data_buffer buffer;

  if (total_size < this->mtu_) {
    if (hops.empty()) {
      if (this->partner_node_id_ == target_node) {
        CHECK_EXPECTED(buffer.add((uint8_t)((uint8_t)protocol_message_type_t::SingleData | message_type)));
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 24)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 16)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 8)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) & 0xFF)));//1
      }
      else {
          CHECK_EXPECTED(buffer.add((uint8_t)((uint8_t)protocol_message_type_t::RouteSingleData | message_type)));
          CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 24)));//1
          CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 16)));//1
          CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 8)));//1
          CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) & 0xFF)));//1
          CHECK_EXPECTED(buffer.add(target_node));
      }
    }
    else {
        CHECK_EXPECTED(buffer.add((uint8_t)((uint8_t)protocol_message_type_t::ProxySingleData | message_type)));
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 24)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 16)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 8)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) & 0xFF)));//1
        CHECK_EXPECTED(buffer.add(target_node));
        CHECK_EXPECTED(buffer.add((uint8_t)hops.size()));
      for (const auto& hop : hops) {
          CHECK_EXPECTED(buffer.add(hop));
      }
    }

    CHECK_EXPECTED(buffer.add(message));
    GET_EXPECTED(sig, hmac::signature(
      this->session_key_,
      hash::sha256(),
      buffer.data(),
      buffer.size()));
    CHECK_EXPECTED(buffer.add(sig));

    const_data_buffer datagram = buffer.move_data();
    vds_assert(datagram.size() <= this->mtu_);

    this->output_messages_.emplace(this->last_output_index_, datagram);
    this->sp_->template get<logger>()->trace(
      ThisModule,
      "%s->%s[%d] sent %d(%d)",
      base64::from_bytes(this->this_node_id_).c_str(),
      base64::from_bytes(this->partner_node_id_).c_str(),
      this->last_output_index_,
      (int)datagram.data()[0],
      this->output_messages_.size());

    this->last_output_index_++;
  }
  else {
    uint32_t offset;

    if (hops.empty()) {
      if (this->partner_node_id_ == target_node) {
        CHECK_EXPECTED(buffer.add((uint8_t)((uint8_t)protocol_message_type_t::Data | message_type)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 24)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 16)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 8)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) & 0xFF)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 24)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 16)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 8)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) & 0xFF)));//1

        CHECK_EXPECTED(buffer.add(message.data(), this->mtu_ - (1 + 4 + SIZE_SIZE + 32)));
        offset = this->mtu_ - (1 + 4 + SIZE_SIZE + 32);
      }
      else {
        CHECK_EXPECTED(buffer.add((uint8_t)((uint8_t)protocol_message_type_t::RouteData | message_type)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 24)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 16)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 8)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) & 0xFF)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 24)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 16)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 8)));//1
        CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) & 0xFF)));//1
        vds_assert(target_node.size() == 32);
        CHECK_EXPECTED(buffer.add(target_node));//32


        CHECK_EXPECTED(buffer.add(message.data(), this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 32)));
        offset = this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 32);
      }
    }
    else {
      CHECK_EXPECTED(buffer.add((uint8_t)((uint8_t)protocol_message_type_t::ProxyData | message_type)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 24)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 16)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) >> 8)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((this->last_output_index_) & 0xFF)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 24)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 16)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) >> 8)));//1
      CHECK_EXPECTED(buffer.add((uint8_t)((message.size()) & 0xFF)));//1
      vds_assert(target_node.size() == 32);
      CHECK_EXPECTED(buffer.add(target_node));//32
      CHECK_EXPECTED(buffer.add((uint8_t)(hops.size())));//1
      for (const auto& hop : hops) {
        vds_assert(hop.size() == 32);
        CHECK_EXPECTED(buffer.add(hop));
      }

      CHECK_EXPECTED(buffer.add(message.data(), this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 1 + hops.size() * 32 + 32)));
      offset = this->mtu_ - (1 + 4 + SIZE_SIZE + 32 + 1 + hops.size() * 32 + 32);
    }

    GET_EXPECTED(sig, hmac::signature(
      this->session_key_,
      hash::sha256(),
      buffer.data(),
      buffer.size()));

    CHECK_EXPECTED(buffer.add(sig));
    const_data_buffer datagram = buffer.move_data();
    vds_assert(datagram.size() <= this->mtu_);

    this->output_messages_.emplace(this->last_output_index_, datagram);
    this->sp_->template get<logger>()->trace(
      ThisModule,
      "%s->%s[%d] send %d(%d)",
      base64::from_bytes(this->this_node_id_).c_str(),
      base64::from_bytes(this->partner_node_id_).c_str(),
      this->last_output_index_,
      (int)datagram.data()[0],
      this->output_messages_.size());

    this->last_output_index_++;

    for (;;) {
      auto size = this->mtu_ - (1 + 4 + 32);
      if (size > message.size() - offset) {
        size = message.size() - offset;
      }

      resizable_data_buffer buffer;
      CHECK_EXPECTED(buffer.add((uint8_t)protocol_message_type_t::ContinueData));//1
      CHECK_EXPECTED(buffer.add(this->last_output_index_ >> 24));//1
      CHECK_EXPECTED(buffer.add(this->last_output_index_ >> 16));//1
      CHECK_EXPECTED(buffer.add(this->last_output_index_ >> 8));//1
      CHECK_EXPECTED(buffer.add(this->last_output_index_));//1
      CHECK_EXPECTED(buffer.add(message.data() + offset, size));//

      GET_EXPECTED(sig, hmac::signature(
        this->session_key_,
        hash::sha256(),
        buffer.data(),
        buffer.size()));
      CHECK_EXPECTED(buffer.add(sig));

      const_data_buffer datagram = buffer.move_data();

      vds_assert(datagram.size() <= this->mtu_);

      this->output_messages_.emplace(this->last_output_index_, datagram);
      this->sp_->template get<logger>()->trace(
        ThisModule,
        "%s->%s[%d] send %d(%d)",
        base64::from_bytes(this->this_node_id_).c_str(),
        base64::from_bytes(this->partner_node_id_).c_str(),
        this->last_output_index_,
        (int)datagram.data()[0],
        this->output_messages_.size());

      this->last_output_index_++;

      if (offset + size >= message.size()) {
        break;
      }
      offset += size;
    }
  }

  vds_assert(start_index < this->last_output_index_);

  if (this->last_output_index_ < start_index) {
    this->failed_state_ = true;
    return make_unexpected<std::runtime_error>("Overflow pipeline");
  }
  auto final_index = this->last_output_index_;
  return std::make_tuple(start_index, final_index);
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::continue_process_messages(const std::shared_ptr<iudp_transport>& s) {

  for (;;) {
    std::unique_lock<std::mutex> lock(this->input_mutex_);

    while (this->input_messages_.end() != this->input_messages_.find(this->last_input_index_)) {
      this->last_input_index_++;
    }

    auto p = this->input_messages_.begin();
    if (p == this->input_messages_.end() || p->first > this->expected_index_) {
      co_return expected<void>();
    }

    if (p->first < this->expected_index_) {
      this->input_messages_.erase(p);
      continue;
    }

    //auto p = this->input_messages_.find(this->expected_index_);
    //if (p == this->input_messages_.end()) {
    //  co_return expected<void>();
    //}


    switch (static_cast<protocol_message_type_t>((uint8_t)protocol_message_type_t::SpecialCommand & *p->second.data())) {
    case protocol_message_type_t::SingleData:
    case protocol_message_type_t::RouteSingleData:
    case protocol_message_type_t::ProxySingleData: {
      auto message_type = (uint8_t)(p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand);

      const_data_buffer target_node;
      const_data_buffer source_node;
      std::vector<const_data_buffer> hops;
      hops.push_back(this->partner_node_id_);
      const_data_buffer message;

      switch (
        static_cast<protocol_message_type_t>(
        (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
        ) {
      case protocol_message_type_t::SingleData: {
        target_node = this->this_node_id_;

        message = const_data_buffer(p->second.data() + 1 + 4, p->second.size() - 1 - 4 - 32);
        break;
      }

      case protocol_message_type_t::RouteSingleData: {
        target_node = const_data_buffer(p->second.data() + 1 + 4, 32);
        message = const_data_buffer(p->second.data() + 1 + 4 + 32, p->second.size() - 1 - 4 - 32 - 32);
        break;
      }

      case protocol_message_type_t::ProxySingleData: {
        target_node = const_data_buffer(p->second.data() + 1 + 4, 32);

        auto hops_count = p->second.data()[1 + 4 + 32];
        for (int i = 0; i < hops_count; ++i) {
          hops.push_back(const_data_buffer(p->second.data() + 1 + 4 + 32 + 1 + i * 32, 32));
        }

        message = const_data_buffer(p->second.data() + 1 + 4 + 32 + 1 + hops_count * 32, p->second.size() - (1 + 4 + 32 + 1 + 32 + hops_count * 32));
        break;
      }
      default:{
        vds_assert(false);
        break;
      }
      }

      const auto message_size = message.size();
      this->expected_index_++;
      this->last_processed_ = std::chrono::steady_clock::now();
      lock.unlock();

      this->process_message(
        s,
        message_type,
        target_node,
        hops,
        message).then([pthis = this->shared_from_this(), message_type, target_node, source_node, message_size](expected<bool> is_good) {
        std::unique_lock<std::mutex> traffic_lock(pthis->traffic_mutex_);
        if (is_good.has_value() && is_good.value()) {
          pthis->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].good_count_++;
          pthis->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].good_traffic_ += message_size;
        }
        else {
          pthis->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].bad_count_++;
          pthis->traffic_[base64::from_bytes(source_node)][base64::from_bytes(target_node)][message_type].bad_traffic_ += message_size;
        }
        traffic_lock.unlock();
      });

      break;

    }

    case protocol_message_type_t::Data:
    case protocol_message_type_t::RouteData:
    case protocol_message_type_t::ProxyData: {
      uint8_t message_type = p->second.data()[0] & ~(uint8_t)protocol_message_type_t::SpecialCommand;
      size_t size =
        uint32_t(p->second.data()[1 + 4] << 24)
        | uint32_t(p->second.data()[1 + 4 + 1] << 16)
        | uint32_t(p->second.data()[1 + 4 + 2] << 8)
        | uint32_t(p->second.data()[1 + 4 + 3]);

      const_data_buffer target_node;

      std::vector<const_data_buffer> hops;
      hops.push_back(this->partner_node_id_);

      resizable_data_buffer message;

      switch (
        static_cast<protocol_message_type_t>(
        (uint8_t)protocol_message_type_t::SpecialCommand & p->second.data()[0])
        ) {
      case protocol_message_type_t::Data: {
        if (size <= p->second.size() - (1 + 4 + SIZE_SIZE + 32)) {
          co_return vds::make_unexpected<std::runtime_error>("Invalid data");
        }
        vds_assert(p->second.size() > (1 + 4 + SIZE_SIZE + 32));
        size -= p->second.size() - (1 + 4 + SIZE_SIZE + 32);

        target_node = this->this_node_id_;
        CHECK_EXPECTED(message.add(p->second.data() + 1 + 4 + SIZE_SIZE, p->second.size() - (1 + 4 + SIZE_SIZE + 32)));
        break;
      }

      case protocol_message_type_t::RouteData: {
        if (size <= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32)) {
          co_return vds::make_unexpected<std::runtime_error>("Invalid data");
        }
        vds_assert(p->second.size() > (1 + 4 + SIZE_SIZE + 32 + 32));
        size -= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32);

        target_node = const_data_buffer(p->second.data() + 1 + 4 + SIZE_SIZE, 32);
        CHECK_EXPECTED(message.add(p->second.data() + (1 + 4 + SIZE_SIZE + 32), p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32)));
        break;
      }

      case protocol_message_type_t::ProxyData: {
        if (size <= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 32 + 1 + 32)) {
          co_return vds::make_unexpected<std::runtime_error>("Invalid data");
        }
        vds_assert(p->second.size() > (1 + 4 + SIZE_SIZE + 32 + 1 + 32));

        target_node = const_data_buffer(p->second.data() + 1 + 4 + SIZE_SIZE, 32);

        auto hops_count = p->second.data()[1 + 4 + SIZE_SIZE + 32];
        for (int i = 0; i < hops_count; ++i) {
          hops.push_back(const_data_buffer(p->second.data() + 1 + 4 + SIZE_SIZE + 32 + 1 + i * 32, 32));
        }

        vds_assert(p->second.size() > (1 + 4 + SIZE_SIZE + 32 + 1 + 32 * hops_count + 32));
        size -= p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 1 + 32 * hops_count + 32);

        CHECK_EXPECTED(message.add(p->second.data() + (1 + 4 + SIZE_SIZE + 32 + 1 + hops_count * 32), p->second.size() - (1 + 4 + SIZE_SIZE + 32 + 1 + hops_count * 32  + 32)));
        break;
      }
      default:
        co_return vds::make_unexpected<std::runtime_error>("Invalid program");
      }

      uint32_t index = 0;
      for (;;) {
        auto p1 = this->input_messages_.find(this->expected_index_ + ++index);
        if (this->input_messages_.end() == p1) {
          co_return expected<void>();
        }

        if ((uint8_t)protocol_message_type_t::ContinueData != p1->second.data()[0]) {
          co_return vds::make_unexpected<std::runtime_error>("Invalid data");
        }

        if (size < p1->second.size() - (1 + 4 + 32)) {
          co_return vds::make_unexpected<std::runtime_error>("Invalid data");
        }

        CHECK_EXPECTED(message.add(p1->second.data() + (1 + 4), p1->second.size() - (1 + 4 + 32)));
        vds_assert(p1->second.size() >(1 + 4 + 32));
        size -= p1->second.size() - (1 + 4 + 32);

        if (0 == size) {
          const auto message_size = message.size();
          this->expected_index_ += index + 1;
          this->last_processed_ = std::chrono::steady_clock::now();
          lock.unlock();

          this->process_message(
            s,
            message_type,
            target_node,
            hops,
            message.move_data()).then([pthis = this->shared_from_this(), message_type, target_node, hops, message_size](expected<bool> is_good) {
            std::unique_lock<std::mutex> traffic_lock(pthis->traffic_mutex_);
            if (is_good.has_value() && is_good.value()) {
              pthis->traffic_[base64::from_bytes(hops[hops.size() - 1])][base64::from_bytes(target_node)][message_type].good_count_++;
              pthis->traffic_[base64::from_bytes(hops[hops.size() - 1])][base64::from_bytes(target_node)][message_type].good_traffic_ += message_size;
            }
            else {
              pthis->traffic_[base64::from_bytes(hops[hops.size() - 1])][base64::from_bytes(target_node)][message_type].bad_count_++;
              pthis->traffic_[base64::from_bytes(hops[hops.size() - 1])][base64::from_bytes(target_node)][message_type].bad_traffic_ += message_size;
            }
            traffic_lock.unlock();
          });

          break;
        }
      }

      break;
    }
    default: {
      //auto p1 = this->input_messages_.find(this->expected_index_ - 4);
      //auto p2 = this->input_messages_.find(this->expected_index_ - 3);
      //auto p3 = this->input_messages_.find(this->expected_index_ - 2);
      //auto p4 = this->input_messages_.find(this->expected_index_ - 1);
      vds_assert(false);
      break;
    }
    }
  }
  co_return expected<void>();
}

vds::async_task<vds::expected<void>> vds::dht::network::dht_datagram_protocol::process_acknowledgment(const std::shared_ptr<iudp_transport>& s, const const_data_buffer& datagram) {

  this->output_mutex_.lock();

  uint32_t last_index =
    (datagram.data()[1] << 24)
    | (datagram.data()[1 + 1] << 16)
    | (datagram.data()[1 + 2] << 8)
    | (datagram.data()[1 + 3]);

  this->sp_->template get<logger>()->trace(
    ThisModule,
    "%s->%s acknowledgment %d",
    base64::from_bytes(this->this_node_id_).c_str(),
    base64::from_bytes(this->partner_node_id_).c_str(),
    last_index);

  for (;;) {
    auto p = this->output_messages_.begin();
    if (this->output_messages_.end() == p) {
      break;
    }

    if (p->first < last_index) {
      this->output_messages_.erase(p);
      this->last_sent_ = 0;
    }
    else {
      break;
    }
  }

  auto p = this->output_messages_.find(last_index);
  if (this->output_messages_.end() != p) {
    udp_datagram datagram(this->address_, p->second);
    this->output_mutex_.unlock();

    this->sp_->template get<logger>()->trace(
      ThisModule,
      "%s->%s resend %d",
      base64::from_bytes(this->this_node_id_).c_str(),
      base64::from_bytes(this->partner_node_id_).c_str(),
      last_index);

    CHECK_EXPECTED_ASYNC(co_await s->write_async(datagram));

    this->output_mutex_.lock();
  }

  uint32_t mask =
    (datagram.data()[5] << 24)
    | (datagram.data()[5 + 1] << 16)
    | (datagram.data()[5 + 2] << 8)
    | (datagram.data()[5 + 3]);

  for (int i = 0; 0 != mask && i < 32; ++i) {
    if (1 == (mask & 1)) {
      p = this->output_messages_.find(last_index + i + 1);
      if (this->output_messages_.end() != p) {
        udp_datagram datagram(this->address_, p->second);
        this->output_mutex_.unlock();

        this->sp_->template get<logger>()->trace(
          ThisModule,
          "%s->%s resend %d",
          base64::from_bytes(this->this_node_id_).c_str(),
          base64::from_bytes(this->partner_node_id_).c_str(),
          last_index + i + 1);

        CHECK_EXPECTED_ASYNC(co_await s->write_async(datagram));

        this->output_mutex_.lock();
      }
    }

    mask >>= 1;
  }

  auto mtu =
    (datagram.data()[8 + 1] << 8)
    | (datagram.data()[8 + 2]);
  if (this->mtu_ < mtu) {
    this->mtu_ = mtu;
  }

  this->output_mutex_.unlock();

  co_return expected<void>();
}
