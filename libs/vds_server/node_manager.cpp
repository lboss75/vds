/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "node_manager.h"

vds::node_manager::node_manager(const service_provider & sp)
  : sp_(sp),
  log_(sp, "Node manager")
{
}

void vds::node_manager::install_prepate(json_array * result, const install_node_prepare & message)
{
  std::unique_lock<std::mutex> lock(this->mutex_);
  for (auto & processed : this->processed_requests_) {
    if (processed == message.request_id) {
      return;//already processed
    }
  }

  this->processed_requests_.push_back(message.request_id);
  while (1000 < this->processed_requests_.size()) {
    this->processed_requests_.pop_front();
  }

  std::string user_folder_name(message.user_id);
  std::replace(user_folder_name.begin(), user_folder_name.end(), ':', '_');

  foldername f(
    foldername(
      foldername(
        persistence::current_user(this->sp_),
        ".vds"),
      "data"),
    user_folder_name);

  install_node_prepared answer;
  answer.user_id = message.user_id;
  answer.request_id = message.request_id;
  answer.new_certificate_serial = std::to_string(std::rand());

  sequence(
    read_file(filename(f, "user.crt")),
    stream_to_string()
  )(
    [&answer](const std::string & body) {
      answer.user_certificate = body;
    },
    [](std::exception * ex) {
      throw ex;
    });

  sequence(
    read_file(filename(f, "user.pkey")),
    stream_to_string()
  )(
   [&answer](const std::string & body) {
      answer.user_private_key = body;
    },
    [](std::exception * ex) {
      throw ex;
    });

  result->add(answer.serialize());
}
