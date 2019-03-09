#include "stdafx.h"
#include "InfoDlg.h"
#include "StringUtils.h"


InfoDlg::InfoDlg(api_void_ptr session)
	: session_(session)
{
}


InfoDlg::~InfoDlg()
{
}

bool InfoDlg::show_dialog(HINSTANCE hinstance)
{
	this->hInst_ = hinstance;
	return (IDOK == DialogBoxParam(hinstance, MAKEINTRESOURCE(IDD_INFO_DIALOG), NULL, DlgProc, reinterpret_cast<LPARAM>(this)));
}

INT_PTR InfoDlg::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

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

			auto pthis = reinterpret_cast<InfoDlg *>(lParam);

			auto balance = vds_get_user_balance(pthis->session_);
			SetDlgItemInt(hDlg, IDC_BALANCE, (UINT)balance, FALSE);

			auto used_str = StringUtils::format_size(pthis->hInst_, vds_get_device_storage_used(pthis->session_));
			auto size_str = StringUtils::format_size(pthis->hInst_, vds_get_device_storage_size(pthis->session_));

			TCHAR message[256];
			LoadString(pthis->hInst_, IDS_SIZE_FORMAT, message, sizeof(message) / sizeof(message[0]));

			DWORD_PTR pArgs[] = { 
				 (DWORD_PTR)used_str.c_str(),
				 (DWORD_PTR)size_str.c_str()
			};
			TCHAR buffer[256];
			DWORD size = sizeof(buffer) / sizeof(buffer[0]);

			FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				message,
				0,
				0,
				buffer,
				size,
				(va_list*)pArgs);

			SetDlgItemText(hDlg, IDC_DISKUSAGE_STATIC, buffer);

			return (INT_PTR)TRUE;
		}

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
		}
		return (INT_PTR)FALSE;
}
