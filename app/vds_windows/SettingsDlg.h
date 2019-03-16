#pragma once
#include "LoginDlg.h"

class SettingsDlg
{
public:
  SettingsDlg();
  ~SettingsDlg();

  bool show(HINSTANCE hinstance);

  bool isDialogMessage(MSG & msg);

private:
  HWND hWnd_;

  static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

};

