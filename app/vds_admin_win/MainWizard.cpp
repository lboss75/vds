// MainWizard.cpp : implementation file
//

#include "stdafx.h"
#include "vds_admin_win.h"
#include "MainWizard.h"


// CMainWizard

IMPLEMENT_DYNAMIC(CMainWizard, CPropertySheet)

CMainWizard::CMainWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
  this->AddPage(&this->first_page_);
  this->AddPage(&this->admin_channel_page_);
}

CMainWizard::CMainWizard(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
  this->AddPage(&this->first_page_);
  this->AddPage(&this->admin_channel_page_);
}

CMainWizard::~CMainWizard()
{
}


BEGIN_MESSAGE_MAP(CMainWizard, CPropertySheet)
END_MESSAGE_MAP()


// CMainWizard message handlers
