#include "stdafx.h"
#include "LoginDlg.h"


LoginDlg::LoginDlg()
{
}


LoginDlg::~LoginDlg()
{
}

bool LoginDlg::show_dialog(HINSTANCE hinstance) {
  return (IDOK == DialogBoxParam(hinstance, MAKEINTRESOURCE(IDD_LOGIN), NULL, DlgProc, reinterpret_cast<LPARAM>(this)));
}


INT_PTR LoginDlg::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

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

	auto pthis = reinterpret_cast<LoginDlg *>(lParam);
	SetDlgItemText(hDlg, IDC_LOGIN_EDIT, pthis->login_.c_str());
	SetDlgItemText(hDlg, IDC_PASSWORD_EDIT, pthis->password_.c_str());

    return (INT_PTR)TRUE;
  }

  case WM_COMMAND:
	  if (LOWORD(wParam) == IDOK)
	  {
		  auto pthis = reinterpret_cast<LoginDlg *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
		  auto len = GetWindowTextLength(GetDlgItem(hDlg, IDC_LOGIN_EDIT));
		  pthis->login_.resize(len + 1);
		  GetDlgItemText(hDlg, IDC_LOGIN_EDIT, const_cast<LPTSTR>(pthis->login_.data()), len + 1);

		  len = GetWindowTextLength(GetDlgItem(hDlg, IDC_PASSWORD_EDIT));
		  pthis->password_.resize(len + 1);
		  GetDlgItemText(hDlg, IDC_PASSWORD_EDIT, const_cast<LPTSTR>(pthis->password_.data()), len + 1);
	  }
	  if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
    {
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}
