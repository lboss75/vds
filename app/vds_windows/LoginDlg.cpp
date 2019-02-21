#include "stdafx.h"
#include "LoginDlg.h"


LoginDlg::LoginDlg()
  : is_authorized_(false)
{
}


LoginDlg::~LoginDlg()
{
}

bool LoginDlg::check_authorized(HINSTANCE hinstance, HWND hParent) {
  if(this->is_authorized_) {
    return true;
  }

  if(IDOK != DialogBoxParam(hinstance, MAKEINTRESOURCE(IDD_LOGIN), hParent, DlgProc, reinterpret_cast<LPARAM>(this))) {
    return false;
  }

  return this->is_authorized_;
}


INT_PTR LoginDlg::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);

  switch (message) {
  case WM_INITDIALOG: {
    SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

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

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}
