#pragma once
#include "vds_admin_winDlg.h"
#include "AdminChannelPage.h"


// CMainWizard

class CMainWizard : public CPropertySheet
{
	DECLARE_DYNAMIC(CMainWizard)

public:
	CMainWizard(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CMainWizard(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CMainWizard();

private:
  Cvds_admin_winDlg first_page_;
  CAdminChannelPage admin_channel_page_;
protected:
	DECLARE_MESSAGE_MAP()
};


