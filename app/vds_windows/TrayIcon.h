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
  HINSTANCE hInst_;
  HWND hWnd_;

  SettingsDlg settingsDls_;

  bool RegisterWndClass();
  bool CreateWnd(int nCmdShow);
  void ShowContextMenu();
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

