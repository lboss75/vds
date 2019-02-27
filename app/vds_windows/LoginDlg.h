#pragma once

class LoginDlg
{
public:
  LoginDlg();
  ~LoginDlg();

  bool show_dialog(HINSTANCE hinstance);

  void login(const std::tstring & value) { this->login_ = value; }
  const std::tstring & login() const { return this->login_; }

  void password(const std::tstring & value) { this->password_ = value; }
  const std::tstring & password() const { return this->password_; }

private:
  static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

  std::tstring login_;
  std::tstring password_;

};

