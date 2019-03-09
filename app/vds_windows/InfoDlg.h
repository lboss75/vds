#pragma once
class InfoDlg
{
public:
	InfoDlg(api_void_ptr session);
	~InfoDlg();

	bool show_dialog(HINSTANCE hinstance);

private:
	api_void_ptr session_;
	HINSTANCE hInst_;

	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

};

