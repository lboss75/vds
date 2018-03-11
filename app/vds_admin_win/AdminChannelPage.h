#pragma once


// CAdminChannelPage dialog

class CAdminChannelPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CAdminChannelPage)

public:
	CAdminChannelPage();
	virtual ~CAdminChannelPage();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADMIN_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
