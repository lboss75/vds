#pragma once
#include "SettingsDlg.h"

class VdsApi;

class TrayIcon
{
public:
  TrayIcon(VdsApi * api);
  ~TrayIcon();

  bool create(HINSTANCE hInst);

  void destroy();

  bool isDialogMessage(MSG & msg);

private:
  VdsApi * api_;
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
  api_void_ptr session_;

  SettingsDlg settingsDls_;

  bool RegisterWndClass();
  bool CreateWnd(int nCmdShow);
  void ShowContextMenu();
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

