#include "stdafx.h"
#include "login_dialog.h"
#include "resource.h"

static INT_PTR CALLBACK login_dialog_message_handler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
    return (INT_PTR)TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDOK:
      {
        login_dialog * instance = reinterpret_cast<login_dialog *>(GetWindowLongPtr(hDlg, GWLP_USERDATA));

        TCHAR login[MAX_PATH + 1];
        GetDlgItemText(hDlg, IDC_LOGIN_EDIT, login, sizeof(login) - 1);

        TCHAR password[MAX_PATH + 1];
        GetDlgItemText(hDlg, IDC_PASSWORD_EDIT, password, sizeof(password) - 1);


        instance->login = login;
        instance->password = password;
        //break;
      }
    case IDCANCEL:
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;

    }
    break;
  }
  return (INT_PTR)FALSE;
}


login_dialog::login_dialog()
{
}


login_dialog::~login_dialog()
{
}

bool login_dialog::show(HINSTANCE hInstance)
{
  return IDOK == DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_LOGIN), NULL, login_dialog_message_handler, (LPARAM)this);
}
