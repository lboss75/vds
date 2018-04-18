// log_parser_app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "LogReader.h"
#include "StringUtils.h"

int _tmain(int argc, TCHAR *argv[]) {
  if(argc != 3) {
    puts("Usage: log_parser file pattern");
    return 1;
  }

  CLogReader reader;
  if(!reader.Open(argv[1])) {
    _tprintf(_T("Open file %s error\n"), argv[1]);
    return 1;
  }

#ifdef _UNICODE
  auto pattern = StringUtils::to_mb(argv[2]);
  if (!reader.SetFilter(pattern)) {
    free(pattern);
    puts("Set pattern error");
    return 1;
  }
  free(pattern);

#else
  if (!reader.SetFilter(argv[2])) {
    puts("Set pattern error");
    return 1;
  }
#endif

  char buffer[1024];
  while(reader.GetNextLine(buffer, sizeof(buffer))) {
    if('\0' == *buffer) {
      return 0;
    }

    puts(buffer);
  }

  return 1;
}

