#include "stdafx.h"
#include "vds_api.h"
#include "vds_embedded.h"


api_void_ptr APICALL vds_init(APIENV_) {
  return static_cast<api_void_ptr>(new vds::vds_embedded());
}

void APICALL vds_done(APIENV api_void_ptr vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  pthis->stop();
  delete pthis;
}

api_string APICALL vds_start(APIENV api_void_ptr vds, int port, bool dev_network) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  try {
    pthis->start(port, dev_network);
    return nullptr;
  }
  catch (const std::exception & ex) {
    pthis->last_error(ex.what());
    return api_return_string(pthis->last_error().c_str());
  }
}


api_string APICALL vds_last_error(APIENV api_void_ptr vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  return api_return_string(pthis->last_error().c_str());
}

void APICALL vds_set_root_folder(APIENV api_void_ptr vds, api_string root_folder_) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  api_string_argument(root_folder, root_folder_);
  pthis->set_root_folder(root_folder);
}

void APICALL vds_server_root(APIENV api_void_ptr vds, api_string login_, api_string password_) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  api_string_argument(login, login_);
  api_string_argument(password, password_);
  pthis->server_root(login, password);
}

api_void_ptr APICALL vds_login(APIENV api_void_ptr vds, api_string login_, api_string password_) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  
  api_string_argument(login, login_);
  api_string_argument(password, password_);

  return pthis->login(login, password);
}

void APICALL vds_session_destroy(APIENV api_void_ptr vds_session) {
  delete static_cast<vds::vds_embedded::vds_session *>(vds_session);
}

api_string APICALL vds_session_check(APIENV api_void_ptr vds_session) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);
  return api_return_string(session->get_login_state());
}

api_string APICALL vds_get_device_storages(APIENV api_void_ptr vds_session) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

  return api_return_string(session->get_device_storages());
}

api_string APICALL vds_prepare_device_storage(APIENV api_void_ptr vds_session) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

  return api_return_string(session->prepare_device_storage());
}

api_string APICALL vds_add_device_storage(APIENV api_void_ptr vds_session, api_string name_, api_string local_path_, int size) {
  auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

  api_string_argument(name, name_);
  api_string_argument(local_path, local_path_);

  return api_return_string(session->add_device_storage(name, local_path, static_cast<uint64_t>(size) * 1024 * 1024 * 1024));
}

api_string APICALL vds_local_storage_exists(APIENV api_void_ptr vds) {
  auto pthis = static_cast<vds::vds_embedded *>(vds);
  return api_return_string(pthis->local_storage_exists() ? "true" : "false");
}


api_string APICALL vds_get_device_storage_path(APIENV api_void_ptr vds_session) {
	auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

	return api_return_string(session->get_device_storage_path());
}

uint64_t APICALL vds_get_device_storage_used(APIENV api_void_ptr vds_session) {
	auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

	return session->get_device_storage_used();
}

uint64_t APICALL vds_get_device_storage_size(APIENV api_void_ptr vds_session) {
	auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

	return session->get_device_storage_size();
}

api_string APICALL vds_set_device_storage_path(APIENV api_void_ptr vds_session, api_string new_path_, uint64_t new_size) {
	auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);
	api_string_argument(new_path, new_path_);
	return api_return_string(session->set_device_storage_path(new_path, new_size));
}

uint64_t APICALL vds_get_user_balance(APIENV api_void_ptr vds_session) {
	auto session = static_cast<vds::vds_embedded::vds_session *>(vds_session);

	return session->get_user_balance();
}
