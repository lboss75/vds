#pragma once

class LoginDlg
{
public:
  LoginDlg();
  ~LoginDlg();

  bool check_authorized(HINSTANCE hinstance, HWND hParent);

private:
  bool is_authorized_;

  static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


};

