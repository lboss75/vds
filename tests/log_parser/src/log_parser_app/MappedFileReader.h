#pragma once

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

  bool is_eof() const {
    return is_eof_;
  }

  bool next_char(char & ch) {
    if(0 == rest_size_) {
      if(is_eof_) {
        ch = '\0';
        return true;
      }

      if(!next_view()) {
        return false;
      }
    }

    ch = *current_position_++;
    --rest_size_;

    return true;
  }

  bool skip_to_newline() {
    char ch;
    while(next_char(ch)) {
      if('\n' == ch) {
        return true;
      }
    }

    return false;
  }

private:
  HANDLE hFile_;
  HANDLE hMapFile_;

  UINT64 view_start_;
  UINT64 readed_;
  bool is_eof_;

  DWORD sys_gran_;

  LPVOID data_;

  const char * current_position_;
  UINT64 rest_size_;

  static constexpr UINT64 VIEW_SIZE = ((UINT64)1024 * 1024 * 1024);

  bool next_view() {
    DWORD dwFileSizeHigh;
    UINT64 file_size_ = GetFileSize(hFile_, &dwFileSizeHigh);
    file_size_ += (((UINT64)dwFileSizeHigh) << 32);

    if(file_size_ <= readed_) {
      is_eof_ = true;
      return true;
    }

    view_start_ = (readed_ / sys_gran_) * sys_gran_;
    auto map_view_size = VIEW_SIZE;
    if(map_view_size > (file_size_ - view_start_)) {
      map_view_size = (file_size_ - view_start_);
    }

    data_ = MapViewOfFile(hMapFile_, FILE_MAP_READ, view_start_ >> 32, view_start_ & 0xFFFFFFFF, map_view_size);
    if (NULL == data_) {
      return false;
    }

    current_position_ = reinterpret_cast<const char *>(data_) + (readed_ - view_start_);
    rest_size_ = map_view_size - (readed_ - view_start_);

    return true;
  }
};

