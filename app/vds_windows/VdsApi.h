#pragma once

class VdsApi
{
public:
  VdsApi();
  ~VdsApi();


  void login(const TCHAR * login, const TCHAR * password);

  std::tstring start();

private:
  api_void_ptr vds_;
  api_void_ptr session_;
};

