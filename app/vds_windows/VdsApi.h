#pragma once

class VdsApi
{
public:
  VdsApi();
  ~VdsApi();

  std::tstring start();

  api_void_ptr api() const { return this->vds_; }

private:
  api_void_ptr vds_;
};

