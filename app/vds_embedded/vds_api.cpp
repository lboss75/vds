#include "stdafx.h"
#include "vds_api.h"
#include "vds_embedded.h"


api_void_ptr vds_init(APIENV_) {
  return (api_void_ptr)new vds::vds_embedded();
}

void vds_done(APIENV api_void_ptr vds) {
  auto pthis = (vds::vds_embedded *)vds;
  delete pthis;
}

api_string vds_last_error(APIENV api_void_ptr vds) {
  auto pthis = (vds::vds_embedded *)vds;
  return api_return_string(pthis->last_error().c_str());
}

void vds_set_root_folder(APIENV api_void_ptr vds, api_string root_folder_) {
  auto pthis = (vds::vds_embedded *)vds;
  api_string_argument(root_folder, root_folder_);
  pthis->set_root_folder(root_folder);
}

void vds_server_root (APIENV api_void_ptr vds, api_string login_, api_string password_) {
  auto pthis = (vds::vds_embedded *)vds;
  api_string_argument(login, login_);
  api_string_argument(password, password_);
  pthis->server_root(login, password);
}
