#pragma once

class InfoDlg
{
public:
	InfoDlg(std::shared_ptr<vds::user_manager> session);
	~InfoDlg();

	bool show_dialog(HINSTANCE hinstance);

private:
  std::shared_ptr<vds::user_manager> session_;
	HINSTANCE hInst_;

	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

};

