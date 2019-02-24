#pragma once
class StringUtils
{
public:
  StringUtils();
  ~StringUtils();

  static std::string to_string(const char * original);
  static std::string to_string(const wchar_t * original);

  static std::tstring from_string(const std::string & original);

};

