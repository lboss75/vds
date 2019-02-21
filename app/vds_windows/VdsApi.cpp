#include "stdafx.h"
#include "VdsApi.h"
#include "StringUtils.h"


VdsApi::VdsApi()
  : session_(nullptr)
{
  this->vds_ = vds_init();
}


VdsApi::~VdsApi()
{
  vds_done(this->vds_);
}

void VdsApi::login(const TCHAR* login, const TCHAR* password) {
  if(this->session_ != nullptr) {
    vds_session_destroy(this->session_);
    this->session_ = nullptr;
  }

  this->session_ = vds_login(this->vds_, StringUtils::to_string(login).c_str(), StringUtils::to_string(password).c_str());

  //API api_string APICALL vds_session_check(APIENV api_void_ptr vds_session);
  //API void APICALL vds_session_destroy(APIENV api_void_ptr vds_session);


}

std::tstring VdsApi::start() {
  auto result = vds_start(this->vds_, 0, true);
  if(nullptr == result) {
    return std::tstring();
  }
  return StringUtils::from_string(result);
}
