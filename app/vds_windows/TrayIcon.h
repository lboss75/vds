#pragma once

class TrayIcon
{
public:
    static const TCHAR* WndClassName;
    static const TCHAR* WndWindowName;

  TrayIcon();
  ~TrayIcon();

  bool create(HINSTANCE hInst);

  void destroy();

private:
  vds::service_registrator registrator_;

  vds::file_logger logger_;
  vds::task_manager task_manager_;
  vds::mt_service mt_service_;
  vds::network_service network_service_;
  vds::crypto_service crypto_service_;
  vds::server server_;
  vds::ws_http_server http_server_;
  const vds::service_provider * sp_;
  HINSTANCE hInst_;
  HWND hWnd_;


  bool RegisterWndClass();
  bool CreateWnd(int nCmdShow);
  void ShowContextMenu();
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

