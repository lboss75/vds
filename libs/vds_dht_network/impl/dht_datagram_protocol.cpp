/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "private/dht_datagram_protocol.h"

static std::mutex sdata_mutex;
static std::map<vds::const_data_buffer, std::map<vds::const_data_buffer, int>> sdata;

void vds::dht::network::tester::register_datagram(const const_data_buffer& data, const const_data_buffer& key) {
  std::lock_guard<std::mutex> locker(sdata_mutex);
  sdata[data][key]++;
}

void vds::dht::network::tester::check_datagram(const const_data_buffer& data, const const_data_buffer& key) {
  std::lock_guard<std::mutex> locker(sdata_mutex);

  vds_assert(sdata.at(data).at(key) > 0 && sdata.at(data).at(key)--);
}
