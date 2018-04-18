#include "stdafx.h"
#include "LogReader.h"
#include "filter_parser.h"


CLogReader::CLogReader()
{
}


CLogReader::~CLogReader()
{
  Close();
}

bool CLogReader::Open(const TCHAR * filename) {
  if(!file_reader_.Open(filename)) {
    return false;
  }

  if (!next_char()) {
    return false;
  }

  return true;
}

inline void CLogReader::Close() {
  file_reader_.Close();
}

bool CLogReader::SetFilter(const char *filter) {
  return filter_parser::parse_filter(filter, filter_);
}

bool CLogReader::GetNextLine(char * buf, const int bufsize) {
  if(nullptr == buf || 0 > bufsize) {
    return false;
  }

  while ('\0' != current_char_) {
    auto state = filter_statemachine::START_STATE;
    auto pbuf = buf;
    auto left_size = bufsize;

    for (;;) {
      // \r\n* -> \n*
      // \r* -> \n*
      auto ch = current_char_;
      if('\r' == current_char_) {
        if(!next_char()) {
          return false;
        }
        ch = '\n';
      }

      state = filter_.next_state(state, ('\n' == ch) ? '\0' : ch);
      if(filter_statemachine::INVALID_STATE == state) {
        break;
      }
      else if(filter_statemachine::FINAL_STATE == state) {
        if ('\n' == ch || '\0' == ch) {
          if('\n' == current_char_) {
            if (!next_char()) {
              return false;
            }
          }
          *pbuf = '\0';
          return true;
        }
        break;
      }

      *pbuf++ = ch;
      --left_size;

      if (0 == left_size) {
        //buffer too small
        break;;
      }

      if (!next_char()) {
        return false;
      }
    }

    if (!file_reader_.skip_to_newline()) {
      return false;
    }

    if (!next_char()) {
      return false;
    }
  }

  *buf = '\0';
  return true;
}

