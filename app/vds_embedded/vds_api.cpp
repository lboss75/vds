#include "stdafx.h"
#include "vds_api.h"
#include "vds_embedded.h"


void * vds_init() {
  return new vds::vds_embedded();
}

void vds_done(void * vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  delete pthis;
}

const char * vds_last_error(void * vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  return pthis->last_error().c_str();
}

void vds_set_root_folder(void * vds, const char * root_folder) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  pthis->set_root_folder(root_folder);
}

void vds_server_root (void * vds, const char * login, const char * password) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  pthis->server_root(login, password);
}
