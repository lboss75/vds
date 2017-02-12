/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_logic.h"

vds::client_logic::client_logic(
  const service_provider & sp,
  certificate & client_certificate,
  asymmetric_private_key & client_private_key)
  : sp_(sp),
  client_certificate_(client_certificate),
  client_private_key_(client_private_key),
  connected_(0)
{
}

void vds::client_logic::start()
{
  auto storage = this->sp_.get<istorage>();

  storage_cursor<endpoint> endpoints(storage);
  while (endpoints.read()) {
    this->connection_queue_.push_back(
      new client_connection<client_logic>(
        this->sp_,
        this,
        endpoints.current().address(),
        endpoints.current().port(),
        this->client_certificate_,
        this->client_private_key_));
  }

  this->update_connection_pool();
}

void vds::client_logic::stop()
{
}

void vds::client_logic::connection_closed(client_connection<client_logic> * connection)
{
  this->connection_mutex_.lock();
  this->connected_--;
  this->connection_mutex_.unlock();

  this->update_connection_pool();
}

void vds::client_logic::connection_error(client_connection<client_logic>* connection, std::exception * ex)
{
  this->connection_mutex_.lock();
  this->connected_--;
  this->connection_mutex_.unlock();

  this->update_connection_pool();
}

void vds::client_logic::node_install(const std::string & login, const std::string & password)
{

}

void vds::client_logic::update_connection_pool()
{
  if (!this->connection_queue_.empty()) {
    std::async(
      std::launch::async,
      [this]() {

      std::chrono::time_point<std::chrono::system_clock> border
        = std::chrono::system_clock::now() - std::chrono::seconds(60);

      std::lock_guard<std::mutex> lock(this->connection_mutex_);
      while (
        !this->sp_.get_shutdown_event().is_shuting_down()
        && this->connected_ < MAX_CONNECTIONS
        && this->connected_ < this->connection_queue_.size()) {

        auto index = std::rand() % this->connection_queue_.size();
        auto connection = this->connection_queue_[index];
        if (
          client_connection<client_logic>::NONE == connection->state()
          || (
            client_connection<client_logic>::CONNECT_ERROR == connection->state()
            && border > connection->connection_end()
            )
          ) {
          connection->connect();
          this->connected_++;
        }
      }
    });
  }
}
