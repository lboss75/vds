#include "stdafx.h"
#include "vds_api.h"
#include "vds_embedded.h"


api_void_ptr vds_init(APIENV_) {
  return static_cast<api_void_ptr>(new vds::vds_embedded());
}

void vds_done(APIENV api_void_ptr vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  delete pthis;
}

api_string vds_start(APIENV api_void_ptr vds, int port) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  try {
    pthis->start(port);
    return nullptr;
  }
  catch (const std::exception & ex) {
    pthis->last_error(ex.what());
    return api_return_string(pthis->last_error().c_str());
  }
}


api_string vds_last_error(APIENV api_void_ptr vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  return api_return_string(pthis->last_error().c_str());
}

void vds_set_root_folder(APIENV api_void_ptr vds, api_string root_folder_) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  api_string_argument(root_folder, root_folder_);
  pthis->set_root_folder(root_folder);
}

void vds_server_root (APIENV api_void_ptr vds, api_string login_, api_string password_) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  api_string_argument(login, login_);
  api_string_argument(password, password_);
  pthis->server_root(login, password);
}

api_void_ptr vds_login(APIENV api_void_ptr vds, api_string login_, api_string password_) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  
  api_string_argument(login, login_);
  api_string_argument(password, password_);

  return pthis->login(login, password);
}

void vds_session_destroy(APIENV api_void_ptr vds_session) {
  delete static_cast<vds::vds_embedded::vds_session *>(vds_session);
}

api_string vds_session_check(APIENV api_void_ptr vds_session) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);
  return api_return_string(session->get_login_state());
}

api_string vds_get_device_storages(APIENV api_void_ptr vds_session) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

  return api_return_string(session->get_device_storages());
}

api_string vds_prepare_device_storage(APIENV api_void_ptr vds_session) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

  return api_return_string(session->prepare_device_storage());
}

api_string vds_add_device_storage(api_void_ptr vds_session, api_string name_, api_string local_path_, int size) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

  api_string_argument(name, name_);
  api_string_argument(local_path, local_path_);

  return api_return_string(session->add_device_storage(name, local_path, static_cast<uint64_t>(size) * 1024 * 1024 * 1024));
}

api_string vds_local_storage_exists(api_void_ptr vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  return api_return_string(pthis->local_storage_exists() ? "true" : "false");
}
