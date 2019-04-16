#pragma once
#include "SettingsDlg.h"

class TrayIcon
{
public:
  TrayIcon();
  ~TrayIcon();

  bool create(HINSTANCE hInst);

  void destroy();

  bool isDialogMessage(MSG & msg);

private:
  vds::service_registrator registrator_;

  vds::file_logger logger_;
  vds::task_manager task_manager_;
  vds::mt_service mt_service_;
  vds::network_service network_service_;
  vds::crypto_service crypto_service_;
  vds::server server_;
  vds::web_server web_server_;
  const vds::service_provider * sp_;
  HINSTANCE hInst_;
  HWND hWnd_;

  enum class AuthState {
	  AS_NOT_INITIED,
	  AS_LOGGING_IN,
	  AS_LOGGED_IN,
	  AS_FAILED
  };

  AuthState auth_state_;
  std::tstring login_;
  std::tstring password_;
  std::shared_ptr<vds::user_manager> session_;

  SettingsDlg settingsDls_;
  int progress_;

  bool RegisterWndClass();
  bool CreateWnd(int nCmdShow);
  void ShowContextMenu();
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  void show_logindlg();

  void update_icon_state();
};

