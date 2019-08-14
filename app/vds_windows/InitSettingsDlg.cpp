#include "stdafx.h"
#include "InitSettingsDlg.h"

InitSettingsDlg::InitSettingsDlg()
{
}


InitSettingsDlg::~InitSettingsDlg()
{
}

bool InitSettingsDlg::show_dialog(HINSTANCE hinstance)
{
  return (IDOK == DialogBoxParam(hinstance, MAKEINTRESOURCE(IDD_INITIAL_SETTINGS), NULL, DlgProc, reinterpret_cast<LPARAM>(this)));
}

bool InitSettingsDlg::update_size() {
  vds::foldername folder(this->storage_path_);

  auto total_size = folder.contains_folder().total_size();
  if (total_size.has_error())
    return false;

  this->reserved_size_ = total_size.value() / 1024 / 1024 / 1706;
  return true;
}


INT_PTR InitSettingsDlg::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

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

    auto pthis = reinterpret_cast<InitSettingsDlg *>(lParam);
    if (!pthis->update_size()) {
      EndDialog(hDlg, LOWORD(IDABORT));
      return (INT_PTR)TRUE;
    }

    SetDlgItemText(hDlg, IDC_LOGIN_EDIT, pthis->login_.c_str());
    SetDlgItemText(hDlg, IDC_PASSWORD_EDIT, pthis->password_.c_str());
    SetDlgItemText(hDlg, IDC_STORAGE_PATH, pthis->storage_path_.c_str());
    SetDlgItemInt(hDlg, IDC_RESERVED_SIZE, pthis->reserved_size_, FALSE);

    return (INT_PTR)TRUE;
  }

  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK)
    {
      auto pthis = reinterpret_cast<InitSettingsDlg *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
      auto len = GetWindowTextLength(GetDlgItem(hDlg, IDC_LOGIN_EDIT));
      pthis->login_.resize(len);
      GetDlgItemText(hDlg, IDC_LOGIN_EDIT, const_cast<LPTSTR>(pthis->login_.data()), len + 1);

      len = GetWindowTextLength(GetDlgItem(hDlg, IDC_PASSWORD_EDIT));
      pthis->password_.resize(len);
      GetDlgItemText(hDlg, IDC_PASSWORD_EDIT, const_cast<LPTSTR>(pthis->password_.data()), len + 1);

      len = GetWindowTextLength(GetDlgItem(hDlg, IDC_STORAGE_PATH));
      pthis->storage_path_.resize(len);
      GetDlgItemText(hDlg, IDC_STORAGE_PATH, const_cast<LPTSTR>(pthis->storage_path_.data()), len + 1);

      pthis->reserved_size_ = GetDlgItemInt(hDlg, IDC_RESERVED_SIZE, NULL, FALSE);

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
