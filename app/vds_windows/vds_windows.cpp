// vds_windows.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "vds_windows.h"
#include "TrayIcon.h"
#include "VdsApi.h"


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(
  _In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPTSTR    lpCmdLine,
  _In_ int       nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  VdsApi api;
  auto error = api.start();
  if(!error.empty()) {
    MessageBox(NULL, error.c_str(), _T("Virtual Data Storage"), MB_ICONERROR);
    return 1;
  }

  TrayIcon trayMenu;
  if(!trayMenu.create(hInstance)) {
    return 2;
  }

  MSG msg;

  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (!trayMenu.isDialogMessage(msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  trayMenu.destroy();

  return (int)msg.wParam;
}

