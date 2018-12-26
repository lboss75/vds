/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "test_vds_dht_network.h"
#include "test_config.h"
#include "../../libs/vds_dht_network/private/udp_transport.h"

int main(int argc, char **argv) {
  vds::dht::network::udp_transport::MAGIC_LABEL = 0xAFAFAF00;

#ifndef _WIN32
  // core dumps may be disallowed by parent of this process; change that
  struct rlimit core_limits;
  core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
  setrlimit(RLIMIT_CORE, &core_limits);
#endif

    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);

    test_config::instance().parse(argc, argv);

    return RUN_ALL_TESTS();
}


