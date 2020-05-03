// vds_windows.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "vds_windows.h"
#include "TrayIcon.h"

// Forward declarations of functions included in this code module:
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

  if (0 == _tcscmp(_T("--stop"), lpCmdLine)) {
      const auto hWnd = FindWindowEx(NULL, NULL, TrayIcon::WndClassName, TrayIcon::WndWindowName);
      if (NULL != hWnd) {
          PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_POPUP_EXIT, 0), 0);
      }

      return 0;
  }

  TrayIcon trayMenu;
  if(!trayMenu.create(hInstance)) {
    return 2;
  }

  MSG msg;

  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  trayMenu.destroy();

  return (int)msg.wParam;
}

