/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vds_background_app.h"
#include "http_router.h"

vds::vds_background_app::vds_background_app()
: http_service_(&router_)
{
  this->router_.add_static(
    "/",
    "<html><body>Hello World</body></html>");
}

void vds::vds_background_app::main(const service_provider & sp)
{
  for (;;) {
    std::cout << "Enter command:\n";

    std::string cmd;
    std::cin >> cmd;

    if ("exit" == cmd) {
      break;
    }
  }
}

void vds::vds_background_app::register_services(vds::service_registrator& registrator)
{
  base::register_services(registrator);
  registrator.add(this->network_service_);
  registrator.add(this->http_service_);
}

static bool ends_with(const std::string & str, const char * suffix)
{

  if (str.empty() || suffix == nullptr) {
    return false;
  }

  size_t str_len = str.size();
  size_t suffix_len = strlen(suffix);

  if (suffix_len > str_len) {
    return false;
  }

  return 0 == strncmp(str.c_str() + str_len - suffix_len, suffix, suffix_len);
}
