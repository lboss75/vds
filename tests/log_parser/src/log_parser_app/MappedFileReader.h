#pragma once

//Memory mappaed file
class CMappedFileReader
{
public:
  CMappedFileReader()
  : hFile_(INVALID_HANDLE_VALUE),
    hMapFile_(NULL),
    view_start_(0),
    data_(NULL),
    rest_size_(0)
  {
  }

  ~CMappedFileReader()
  {
    Close();
  }

  bool Open(const TCHAR * filename) {
    if(nullptr == filename) {
      return false;
    }

    hFile_ = CreateFile(
      filename,
      GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING,
      NULL);

    if (INVALID_HANDLE_VALUE == hFile_) {
      return false;
    }

    hMapFile_ = CreateFileMapping(hFile_, NULL, PAGE_READONLY, 0, 0, NULL);
    if (NULL == hMapFile_) {
      return false;
    }

    // Get the system allocation granularity.
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    sys_gran_ = sys_info.dwAllocationGranularity;
    return true;
  }

  void    Close() {
    if (NULL != data_) {
      UnmapViewOfFile(data_);
      data_ = NULL;
    }

    if (NULL != hMapFile_) {
      CloseHandle(hMapFile_);
      hMapFile_ = NULL;
    }

    if (INVALID_HANDLE_VALUE != hFile_) {
      CloseHandle(hFile_);
      hFile_ = INVALID_HANDLE_VALUE;
    }
  }

  //Is end of file
  bool is_eof() const {
    return is_eof_;
  }

  //Get next char or '\0' if eof
  bool next_char(char & ch) {
    if(0 == rest_size_ && !is_eof_) {
      if(!next_view()) {
        return false;
      }
    }

    if(is_eof_) {
      ch = '\0';
      return true;
    }

    ch = *current_position_++;
    --rest_size_;

    return true;
  }

  //read to end of line
  bool skip_to_newline() {
    char ch;
    while(next_char(ch)) {
      if('\n' == ch || '\0' == ch) {
        return true;
      }
    }

    return false;
  }

private:
  //File handles
  HANDLE hFile_;
  HANDLE hMapFile_;

  //View parameters
  UINT64 view_start_;
  DWORD sys_gran_;

  //Current view data
  LPVOID data_;

  //readed butes
  UINT64 readed_;

  //end of file flag
  bool is_eof_;
  
  //current symbol position
  const char * current_position_;

  //rest symbols in the view
  SIZE_T rest_size_;

  static constexpr SIZE_T VIEW_SIZE = ((SIZE_T)1024 * 1024 * 1024);

  bool next_view() {
    if (NULL != data_) {
      UnmapViewOfFile(data_);
      data_ = NULL;
    }

    DWORD dwFileSizeHigh;
    UINT64 file_size_ = GetFileSize(hFile_, &dwFileSizeHigh);
    if(INVALID_FILE_SIZE == file_size_) {
      return false;
    }

    file_size_ += (((UINT64)dwFileSizeHigh) << 32);

    if(file_size_ <= readed_) {
      is_eof_ = true;
      return true;
    }

    view_start_ = (readed_ / sys_gran_) * sys_gran_;
    auto map_view_size = VIEW_SIZE;
    if(map_view_size > (file_size_ - view_start_)) {
      map_view_size = (SIZE_T)(file_size_ - view_start_);
    }

    data_ = MapViewOfFile(hMapFile_, FILE_MAP_READ, view_start_ >> 32, view_start_ & 0xFFFFFFFF, map_view_size);
    if (NULL == data_) {
      return false;
    }

    current_position_ = reinterpret_cast<const char *>(data_) + (readed_ - view_start_);
    rest_size_ = map_view_size - (readed_ - view_start_);
    readed_ = view_start_ + map_view_size;

    return true;
  }
};

