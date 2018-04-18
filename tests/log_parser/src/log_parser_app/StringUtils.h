#pragma once
class StringUtils
{
public:
  //Convert from WIDE char to multibyte
  static char * to_mb(const wchar_t * str) {
    auto size = wcstombs(nullptr, str, 0);
    if(size < 0) {
      return nullptr;
    }

    char * result = (char *)malloc(size + 1);
    if(nullptr == result) {
      return nullptr;
    }

    wcstombs(result, str, size + 1);
    return result;
  }
};

