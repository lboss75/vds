// AdminChannelPage.cpp : implementation file
//

#include "stdafx.h"
#include "vds_admin_win.h"
#include "AdminChannelPage.h"
#include "afxdialogex.h"


// CAdminChannelPage dialog

IMPLEMENT_DYNAMIC(CAdminChannelPage, CPropertyPage)

CAdminChannelPage::CAdminChannelPage()
	: CPropertyPage(IDD_ADMIN_DIALOG)
{

}

CAdminChannelPage::~CAdminChannelPage()
{
}

void CAdminChannelPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAdminChannelPage, CPropertyPage)
END_MESSAGE_MAP()


// CAdminChannelPage message handlers
