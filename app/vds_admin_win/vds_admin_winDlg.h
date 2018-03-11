
// vds_admin_winDlg.h : файл заголовка
//

#pragma once


// диалоговое окно Cvds_admin_winDlg
class Cvds_admin_winDlg : public CPropertyPage
{
// Создание
public:
	Cvds_admin_winDlg(CWnd* pParent = NULL);	// стандартный конструктор

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VDS_ADMIN_WIN_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// поддержка DDX/DDV


// Реализация
protected:
	HICON m_hIcon;

	// Созданные функции схемы сообщений
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedRadio2();
  virtual BOOL OnSetActive();
};
