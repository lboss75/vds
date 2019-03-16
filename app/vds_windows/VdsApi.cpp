#include "stdafx.h"
#include "VdsApi.h"
#include "StringUtils.h"


VdsApi::VdsApi()
{
  this->vds_ = vds_init();
}


VdsApi::~VdsApi()
{
  vds_done(this->vds_);
}

std::tstring VdsApi::start() {
  auto result = vds_start(this->vds_, 0, true);
  if(nullptr == result) {
    return std::tstring();
  }
  return StringUtils::from_string(result);
}
