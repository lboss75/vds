#pragma once
class login_dialog
{
public:
  login_dialog();
  ~login_dialog();

  bool show(HINSTANCE hInstance);

  std::wstring login;
  std::wstring password;
};

