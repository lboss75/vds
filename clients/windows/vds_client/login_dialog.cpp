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

        TCHAR file_name[MAX_PATH + 1];
        GetDlgItemText(hDlg, IDC_FILE_EDIT, file_name, sizeof(file_name) - 1);

        TCHAR password[MAX_PATH + 1];
        GetDlgItemText(hDlg, IDC_PASSWORD_EDIT, password, sizeof(password) - 1);


        instance->file_name = file_name;
        instance->password = password;
        //break;
      }
    case IDCANCEL:
      EndDialog(hDlg, LOWORD(wParam));
      return (INT_PTR)TRUE;

    case IDC_SELECTFILE_BUTTON:
      {
        TCHAR file_name[MAX_PATH + 1];
        GetDlgItemText(hDlg, IDC_FILE_EDIT, file_name, sizeof(file_name) - 1);

        OPENFILENAME ofn;

        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hDlg;
        ofn.lpstrDefExt = _T(".vdsi");
        ofn.lpstrFile = file_name;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = _T("Приглашение (*.vdsi)\0*.vdsi\0Все файлы\0*.*\0");
        ofn.nFilterIndex = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = _T("Выберите файл приглашения");
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        GetOpenFileName(&ofn);

        if (0 < _tcslen(ofn.lpstrFile)) {
          SetDlgItemText(hDlg, IDC_FILE_EDIT, ofn.lpstrFile);
        }

        return (INT_PTR)TRUE;
      }
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
