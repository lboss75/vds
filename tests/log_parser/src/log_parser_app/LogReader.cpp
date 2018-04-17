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

bool CLogReader::SetFilter(const char *filter) {
  return filter_parser::parse_filter(filter, filter_);
}

bool CLogReader::GetNextLine(char * buf, const int bufsize) {

  for (;;) {
    auto state = filter_statemachine::START_STATE;
    auto pbuf = buf;
    auto left_size = bufsize;

    char ch;
    for (;;) {
      if(!file_reader_.next_char(ch)) {
        return false;
      }

      if('\r' == ch || '\n' == ch) {
        ch = '\0';
      }

      state = filter_.next_state(state, ch);
      if(filter_statemachine::INVALID_STATE == state) {
        break;
      }
      else if(filter_statemachine::FINAL_STATE == state) {
        if ('\0' == ch) {
          return true;
        }
        break;
      }
      if(0 == left_size) {
        //buffer too small
        break;;
      }
      *pbuf++ = ch;
      --left_size;
    }

    if('\0' == ch) {
      continue;      
    }

    if (!file_reader_.skip_to_newline()) {
      return false;
    }
  }
}
