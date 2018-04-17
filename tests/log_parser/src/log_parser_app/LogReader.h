#pragma once

#include "MappedFileReader.h"
#include "filter_statemachine.h"

class CLogReader
{
public:
  CLogReader();
  ~CLogReader();

  bool    Open(const TCHAR * filename);                         // открытие файла, false - ошибка
  void    Close();                        // закрытие файла

  bool    SetFilter(const char *filter);  // установка фильтра строк, false - ошибка
  bool    GetNextLine(char *buf,          // запрос очередной найденной строки, 
    const int bufsize);                   // buf - буфер, bufsize - максимальная длина
                                          // false - конец файла или ошибка};
private:
  CMappedFileReader file_reader_;
  filter_statemachine filter_;
};

inline bool CLogReader::Open(const TCHAR * filename) {
  return file_reader_.Open(filename);
}

inline void CLogReader::Close() {
  file_reader_.Close();
}

