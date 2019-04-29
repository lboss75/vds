#include "stdafx.h"
#include "StringUtils.h"


StringUtils::StringUtils()
{
}


StringUtils::~StringUtils()
{
}

std::string StringUtils::to_string(const char * original) {
  auto buf_size = ::MultiByteToWideChar(CP_ACP, 0, original, -1, NULL, 0);
  auto utf16 = new WCHAR[buf_size];

  ::MultiByteToWideChar(CP_ACP, 0, original, -1, utf16, buf_size);

  buf_size = ::WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL);
  auto utf8 = new CHAR[buf_size];

  ::WideCharToMultiByte(CP_UTF8, 0, utf16, -1, utf8, buf_size, NULL, NULL);

  std::string result(utf8, ::strlen(utf8));
  delete[] utf8;
  delete[] utf16;

  return result;
}

std::string StringUtils::to_string(const wchar_t * original) {
  auto buf_size = ::WideCharToMultiByte(CP_UTF8, 0, original, -1, NULL, 0, NULL, NULL);
  auto utf8 = new CHAR[buf_size];

  ::WideCharToMultiByte(CP_UTF8, 0, original, -1, utf8, buf_size, NULL, NULL);

  std::string result(utf8, ::strlen(utf8));
  delete[] utf8;

  return result;
}

std::tstring StringUtils::from_string(const std::string& original) {
  auto buf_size = ::MultiByteToWideChar(CP_UTF8, 0, original.c_str(), -1, NULL, 0);
  auto utf16 = new WCHAR[buf_size];

  ::MultiByteToWideChar(CP_UTF8, 0, original.c_str(), -1, utf16, buf_size);

#ifdef _UNICODE
  std::wstring result(utf16, ::wcslen(utf16));
#else

  buf_size = ::WideCharToMultiByte(CP_ACP, 0, utf16, -1, NULL, 0, NULL, NULL);
  auto utf8 = new CHAR[buf_size];

  ::WideCharToMultiByte(CP_ACP, 0, utf16, -1, utf8, buf_size, NULL, NULL);

  std::string result(utf8, ::strlen(utf8));

  delete[] utf8;
#endif

  delete[] utf16;

  return result;
}

std::tstring StringUtils::format_size(HINSTANCE hInstance, uint64_t size)
{
	TCHAR postfixes[256];
	LoadString(hInstance, IDS_SIZE_POSTFIXES, postfixes, sizeof(postfixes) / sizeof(postfixes) - 1);

	TCHAR * p = postfixes;
	while (size > 1024) {
		size /= 1024;
		p = _tcschr(p, _T(',')) + 1;
	}

	*_tcschr(p, _T(',')) = _T('\0');

#ifdef _UNICODE
	return std::to_wstring(size) + L" " + p;
#else
	return std::to_string(size) + " " + p;
#endif
}
