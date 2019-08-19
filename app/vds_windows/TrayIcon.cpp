#include "stdafx.h"
#include "TrayIcon.h"
#include "StringUtils.h"
#include "InitSettingsDlg.h"

TrayIcon::TrayIcon()
: auth_state_(AuthState::AS_NOT_INITIED),
	progress_(0),
  logger_(vds::log_level::ll_trace, std::unordered_set<std::string>{"*"})
{
  this->registrator_.add(this->mt_service_);
  this->registrator_.add(this->logger_);
  this->registrator_.add(this->task_manager_);
  this->registrator_.add(this->network_service_);
  this->registrator_.add(this->crypto_service_);
  this->registrator_.add(this->server_);
  this->registrator_.add(this->web_server_);
}


TrayIcon::~TrayIcon()
{
  (void)this->registrator_.shutdown();
}

bool TrayIcon::create(HINSTANCE hInst) {

  auto sp = this->registrator_.build();
  if(!check(sp)) {
    return false;
  }
  this->sp_ = sp.value();

  auto start_result = this->registrator_.start();
  if (!check(start_result)) {
    return false;
  }

  start_result = this->server_.start_network(8050, false).get();
  if (!check(start_result)) {
    return false;
  }

  this->hInst_ = hInst;
  this->RegisterWndClass();

  if (!this->CreateWnd(SW_HIDE)) {
    return false;
  }

  SetTimer(this->hWnd_, 1, 5000, NULL);

  NOTIFYICONDATA nid = {};
  nid.cbSize = sizeof(nid);
  nid.hWnd = this->hWnd_;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE;
  nid.uCallbackMessage = WM_APP + 1;

  nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));

  if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
	  return false;
  }


  nid.uVersion = NOTIFYICON_VERSION_4;
  Shell_NotifyIcon(NIM_SETVERSION, &nid);


  // For a simple Tip
  nid.uFlags = NIF_TIP | NIF_SHOWTIP;
  LoadString(hInst, IDS_INIT_TIP, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
  Shell_NotifyIcon(NIM_MODIFY, &nid);

  // For a Ballon Tip
  nid.uFlags = NIF_INFO | NIF_SHOWTIP;
  nid.dwInfoFlags = NIIF_INFO;
  LoadString(hInst, IDS_INIT_TIP, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
  LoadString(hInst, IDS_INIT_TIP_TITLE, nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]) - 1);
  nid.uTimeout = 5000;

  Shell_NotifyIcon(NIM_MODIFY, &nid);

  bool disk_allocated = false;
  auto result = this->sp_->get<vds::db_model>()->async_read_transaction(
    [this, &disk_allocated](vds::database_read_transaction & t) -> vds::expected<void> {

    auto client = this->sp_->get<vds::dht::network::client>();

    vds::orm::current_config_dbo t1;
    GET_EXPECTED(st, t.get_reader(
      t1
      .select(t1.owner_id)
      .where(t1.node_id == client->current_node_id())));
    GET_EXPECTED(st_result, st.execute());

    if (st_result && t1.owner_id.get(st).size() > 0) {
      disk_allocated = true;
    }

    return vds::expected<void>();
  }).get();

  if (!check(result)) {
    return false;
  }

  if (!disk_allocated) {
    return this->show_InitialSettings();
  }

  return true;
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

  auto menu = LoadMenu(this->hInst_, MAKEINTRESOURCE((this->auth_state_ == AuthState::AS_LOGGED_IN) ? IDR_TRAYMENU : IDR_NOTAUTH_MENU));
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
    switch (LOWORD(lParam))
    {
	case WM_LBUTTONUP: {
		auto pthis = reinterpret_cast<TrayIcon *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    auto url = std::string("http://localhost:") + std::to_string(pthis->sp_->get<vds::dht::network::client>()->port());
    ShellExecuteA(0, 0, url.c_str(), 0, 0, SW_SHOW);
		break;
	}
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
	case ID_POPUP_LOGIN: {
		auto pthis = reinterpret_cast<TrayIcon *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		pthis->show_logindlg();
		break;
	}
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
  case WM_TIMER: {
	  auto pthis = reinterpret_cast<TrayIcon *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	  switch (pthis->auth_state_){
	  case AuthState::AS_LOGGING_IN: {
		  auto result = pthis->session_->get_login_state();
		  if (result == vds::user_manager::login_state_t::login_successful) {
			  pthis->auth_state_ = AuthState::AS_LOGGED_IN;
		  }
		  else if (result == vds::user_manager::login_state_t::login_failed) {
			  pthis->auth_state_ = AuthState::AS_FAILED;
		  }
		  pthis->update_icon_state();
		  break;
	  }

	  default:
		  break;
	  }

	  break;
  }
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

void TrayIcon::show_logindlg()
{
	LoginDlg dlg;
	dlg.login(this->login_);
	dlg.password(this->password_);
	if (dlg.show_dialog(this->hInst_)) {
		if (this->session_ != nullptr) {
			this->session_.reset();
		}

		this->login_ = dlg.login();
		this->password_ = dlg.password();
		this->auth_state_ = AuthState::AS_LOGGING_IN;
    this->session_ = std::make_shared<vds::user_manager>(this->sp_);
    auto result = this->sp_->get<vds::db_model>()->async_read_transaction(
      [this](vds::database_read_transaction & t) -> vds::expected<void> {
      return this->session_->load(t, StringUtils::to_string(this->login_.c_str()), StringUtils::to_string(this->password_.c_str()));
    }).get();

		this->update_icon_state();
	}
}

void TrayIcon::update_icon_state()
{
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = this->hWnd_;
	nid.uID = 1;

	switch (this->auth_state_)
	{
	case AuthState::AS_NOT_INITIED: {
		// For a simple Tip
		nid.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_ICON;
		nid.hIcon = LoadIcon(this->hInst_, MAKEINTRESOURCE(IDI_SMALL));

		LoadString(this->hInst_, IDS_INIT_TIP, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		Shell_NotifyIcon(NIM_MODIFY, &nid);

		// For a Ballon Tip
		nid.uFlags = NIF_INFO | NIF_SHOWTIP;
		nid.dwInfoFlags = NIIF_INFO;
		LoadString(this->hInst_, IDS_INIT_TIP, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		LoadString(this->hInst_, IDS_INIT_TIP_TITLE, nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]) - 1);
		nid.uTimeout = 5000;

		break;
	}
	case AuthState::AS_LOGGING_IN: {
		// For a simple Tip
		nid.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_ICON;
		switch (this->progress_++)
		{
		case 0:
			nid.hIcon = LoadIcon(this->hInst_, MAKEINTRESOURCE(IDI_PROGRESS_1));
			break;
		case 1:
			nid.hIcon = LoadIcon(this->hInst_, MAKEINTRESOURCE(IDI_PROGRESS_2));
			break;
		default:
			nid.hIcon = LoadIcon(this->hInst_, MAKEINTRESOURCE(IDI_PROGRESS_3));
			this->progress_ = 0;
			break;
		}
		LoadString(this->hInst_, IDS_LOGING_IN, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		Shell_NotifyIcon(NIM_MODIFY, &nid);

		// For a Ballon Tip
		nid.uFlags = NIF_INFO | NIF_SHOWTIP;
		nid.dwInfoFlags = NIIF_INFO;
		LoadString(this->hInst_, IDS_LOGING_IN, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		LoadString(this->hInst_, IDS_INIT_TIP_TITLE, nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]) - 1);
		nid.uTimeout = 5000;

		break;
	}
	case AuthState::AS_LOGGED_IN: {
		// For a simple Tip
		nid.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_ICON;
		nid.hIcon = LoadIcon(this->hInst_, MAKEINTRESOURCE(IDI_SMALL_GREEN));
		LoadString(this->hInst_, IDS_LOGGED_IN, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		Shell_NotifyIcon(NIM_MODIFY, &nid);

		// For a Ballon Tip
		nid.uFlags = NIF_INFO | NIF_SHOWTIP;
		nid.dwInfoFlags = NIIF_INFO;
		LoadString(this->hInst_, IDS_LOGGED_IN, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		LoadString(this->hInst_, IDS_INIT_TIP_TITLE, nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]) - 1);
		nid.uTimeout = 5000;

		break;
	}
	case AuthState::AS_FAILED: {
		// For a simple Tip
		nid.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_ICON;
		nid.hIcon = LoadIcon(this->hInst_, MAKEINTRESOURCE(IDI_SMALL_RED));
		LoadString(this->hInst_, IDS_LOGIN_FAILED, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		Shell_NotifyIcon(NIM_MODIFY, &nid);

		// For a Ballon Tip
		nid.uFlags = NIF_INFO | NIF_SHOWTIP;
		nid.dwInfoFlags = NIIF_INFO;
		LoadString(this->hInst_, IDS_LOGIN_FAILED, nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
		LoadString(this->hInst_, IDS_INIT_TIP_TITLE, nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]) - 1);
		nid.uTimeout = 5000;

		break;
	}
	}

	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

bool TrayIcon::show_InitialSettings()
{
  auto current_user = vds::persistence::current_user(this->sp_);
  if (!check(current_user)) {
    return false;
  }

  auto storage_folder = vds::foldername(current_user.value(), "storage").local_name();

  for (;;) {
    InitSettingsDlg dlg;

    dlg.login(this->login_);
    dlg.password(this->password_);
    dlg.storage_path(storage_folder);

    if (!dlg.show_dialog(this->hInst_)) {
      return false;
    }

    this->login_ = dlg.login();
    this->password_ = dlg.password();
    this->auth_state_ = AuthState::AS_LOGGING_IN;
    this->session_ = std::make_shared<vds::user_manager>(this->sp_);
    auto result = this->sp_->get<vds::db_model>()->async_read_transaction(
      [this](vds::database_read_transaction & t) -> vds::expected<void> {
      return this->session_->load(t, StringUtils::to_string(this->login_.c_str()), StringUtils::to_string(this->password_.c_str()));
    }).get();

    if (!check(result)) {
      return false;
    }

    if(vds::user_manager::login_state_t::login_successful == this->session_->get_login_state()) {
      this->auth_state_ = AuthState::AS_LOGGED_IN;
      this->update_icon_state();

      vds::foldername f(dlg.storage_path());
      
      if (!check(f.create())) {
        return false;
      }

      auto body = vds::user_storage::device_storage_label(this->session_);
      if (!check(body)) {
        return false;
      }

      auto file_body = body.value()->str();
      if (!check(file_body)) {
        return false;
      }

      vds::filename fn(f, ".vds_storage.json");
      if (!check(vds::file::write_all(fn, vds::const_data_buffer(file_body.value().c_str(), file_body.value().size())))) {
        return false;
      }

      if (!check(vds::user_storage::set_device_storage(this->sp_, this->session_, dlg.storage_path(), dlg.reserved_size() * 1024 * 1024 * 1024).get())) {
        return false;
      }

      return true;
    }
    else {
      MessageBoxA(NULL, "Логин или пароль не зарегистрированы", "Виртуальное хранение файлов", MB_ICONERROR);
    }
  }
  return false;
}

bool TrayIcon::isDialogMessage(MSG& msg) {
  return this->settingsDls_.isDialogMessage(msg);
}
