#include "stdafx.h"
#include "SettingsDlg.h"


SettingsDlg::SettingsDlg()
  : hWnd_(NULL)
{
}


SettingsDlg::~SettingsDlg()
{
}

bool SettingsDlg::show(HINSTANCE hinstance) {
  if (NULL == this->hWnd_) {
    this->hWnd_ = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_SETTINGS), NULL, DlgProc);
    SetWindowLongPtr(this->hWnd_, GWLP_USERDATA, reinterpret_cast<INT_PTR>(this));
  }

  ShowWindow(this->hWnd_, SW_SHOW);
  SetForegroundWindow(this->hWnd_);

  return true;
}

bool SettingsDlg::isDialogMessage(MSG& msg) {
  if(NULL != this->hWnd_) {
    return IsDialogMessage(this->hWnd_, &msg);
  }

  return false;
}

INT_PTR SettingsDlg::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);

  switch (message) {
  case WM_INITDIALOG: {
    //Center Window
    RECT desktopRect;
    GetClientRect(GetDesktopWindow(), &desktopRect);

    RECT rc;
    GetWindowRect(hDlg, &rc);

    SetWindowPos(
      hDlg,
      0,
      (desktopRect.right - desktopRect.left - rc.right + rc.left) / 2,
      (desktopRect.bottom - desktopRect.top - rc.bottom + rc.top) / 2,
      0,
      0,
      SWP_NOZORDER | SWP_NOSIZE);

    return (INT_PTR)TRUE;
  }

  case WM_CLOSE:
    ShowWindow(hDlg, SW_HIDE);
    return (INT_PTR)TRUE;

  //case WM_COMMAND:
  //  if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
  //  {
  //    EndDialog(hDlg, LOWORD(wParam));
  //    return (INT_PTR)TRUE;
  //  }
  //  break;
  }
  return (INT_PTR)FALSE;

}
