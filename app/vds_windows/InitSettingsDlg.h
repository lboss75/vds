#pragma once


class InitSettingsDlg
{
public:
  InitSettingsDlg();
  ~InitSettingsDlg();

  bool show_dialog(HINSTANCE hinstance);

  void login(const std::tstring & value) { this->login_ = value; }
  const std::tstring & login() const { return this->login_; }

  void password(const std::tstring & value) { this->password_ = value; }
  const std::tstring & password() const { return this->password_; }

  void storage_path(const std::tstring & value) { this->storage_path_ = value; }
  const std::tstring & storage_path() const { return this->storage_path_; }

  void reserved_size(UINT value) { this->reserved_size_ = value; }
  UINT reserved_size() const { return this->reserved_size_; }

private:
  static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

  std::tstring login_;
  std::tstring password_;
  std::tstring storage_path_;
  UINT reserved_size_;

  bool update_size();
};

