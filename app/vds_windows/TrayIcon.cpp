#include "stdafx.h"
#include "TrayIcon.h"



TrayIcon::TrayIcon()
{
}


TrayIcon::~TrayIcon()
{
}

bool TrayIcon::create(HINSTANCE hInst) {
  this->hInst_ = hInst;
  this->RegisterWndClass();

  if (!this->CreateWnd(SW_HIDE)) {
    return false;
  }

  NOTIFYICONDATA nid = {};
  nid.cbSize = sizeof(nid);
  nid.hWnd = this->hWnd_;
  nid.uFlags = NIF_ICON | NIF_MESSAGE;
  nid.uCallbackMessage = WM_APP + 1;

  nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));

  return (Shell_NotifyIcon(NIM_ADD, &nid) ? true : false);
}

void TrayIcon::destroy() {
  NOTIFYICONDATA nid = {};
  nid.cbSize = sizeof(nid);
  nid.hWnd = this->hWnd_;

  Shell_NotifyIcon(NIM_DELETE, &nid);
}


bool TrayIcon::RegisterWndClass()
{
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = this->hInst_;
  wcex.hIcon = NULL;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = _T("VDS Windows Class");
  wcex.hIconSm = NULL;

  return (RegisterClassEx(&wcex) != NULL);
}

bool TrayIcon::CreateWnd(int nCmdShow)
{
  this->hWnd_ = CreateWindow(
    _T("VDS Windows Class"),
    _T("VDS Window"),
    WS_OVERLAPPED,
    CW_USEDEFAULT,
    0,
    CW_USEDEFAULT,
    0,
    nullptr,
    nullptr,
    this->hInst_,
    nullptr);

  if (!this->hWnd_) {
    return false;
  }

  SetWindowLongPtr(this->hWnd_, GWLP_USERDATA, reinterpret_cast<INT_PTR>(this));

  ShowWindow(this->hWnd_, nCmdShow);
  UpdateWindow(this->hWnd_);

  return true;
}

void TrayIcon::ShowContextMenu() {
  SetForegroundWindow(this->hWnd_);

  auto menu = LoadMenu(this->hInst_, MAKEINTRESOURCE(IDR_TRAYMENU));
  auto popupMenu = GetSubMenu(menu, 0);

  POINT pt;
  GetCursorPos(&pt);
  TrackPopupMenu(popupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, this->hWnd_, NULL);

  PostMessage(this->hWnd_, WM_NULL, 0, 0);
}

LRESULT CALLBACK TrayIcon::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_APP + 1: {
    switch (lParam)
    {
    case WM_LBUTTONUP:
    //case WM_LBUTTONDBLCLK:
    //  ShowWindow(hWnd, SW_RESTORE);
      break;
    case WM_RBUTTONUP:
    case WM_CONTEXTMENU: {
      auto pthis = reinterpret_cast<TrayIcon *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
      pthis->ShowContextMenu();
      break;
    }
    }
    break;
  }
  case WM_COMMAND:
  {
    int wmId = LOWORD(wParam);
    // Parse the menu selections:
    switch (wmId)
    {
    case ID_POPUP_EXIT: {
      PostQuitMessage(0);
      break;
    }

    case ID_POPUP_SETTINGS: {
      auto pthis = reinterpret_cast<TrayIcon *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
      pthis->settingsDls_.show(pthis->hInst_);
      break;
      
    }
      //case IDM_ABOUT:
      //    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
      //    break;
      //case IDM_EXIT:
      //    DestroyWindow(hWnd);
      //    break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  }
  break;
  //case WM_PAINT:
  //    {
  //        PAINTSTRUCT ps;
  //        HDC hdc = BeginPaint(hWnd, &ps);
  //        // TODO: Add any drawing code that uses hdc here...
  //        EndPaint(hWnd, &ps);
  //    }
  //    break;
  //case WM_DESTROY:
  //    PostQuitMessage(0);
  //    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

bool TrayIcon::isDialogMessage(MSG& msg) {
  return this->settingsDls_.isDialogMessage(msg);
}
