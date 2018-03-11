
// vds_admin_win.h : главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "включить stdafx.h до включения этого файла в PCH"
#endif

#include "resource.h"		// основные символы


// Cvds_admin_winApp:
// О реализации данного класса см. vds_admin_win.cpp
//

class Cvds_admin_winApp : public CWinApp
{
public:
	Cvds_admin_winApp();

// Переопределение
public:
	virtual BOOL InitInstance();

// Реализация

	DECLARE_MESSAGE_MAP()
};

extern Cvds_admin_winApp theApp;