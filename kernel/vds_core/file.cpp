#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include <sys/stat.h>
#include "file.h"
#include "persistence.h"

vds::file::file()
: handle_(0)
{
}


vds::file::file(const filename & filename, file_mode mode)
: handle_(0)
{
  this->open(filename, mode);
}

vds::file::file(file&& f) noexcept 
: filename_(f.filename_), handle_(f.handle_){
  f.handle_ = 0;
}

vds::file::~file()
{
  this->close();
}

void vds::file::close()
{
  if(0 != this->handle_){
#ifndef _WIN32
    ::close(this->handle_);
#else
    ::_close(this->handle_);
#endif
    this->handle_ = 0;
  }
}
  
void vds::file::open(const vds::filename& filename, vds::file::file_mode mode)
{
  this->filename_ = filename;
  
  int oflags;
  switch (mode) {
  case file_mode::append:
    oflags = O_RDWR | O_CREAT | O_APPEND;
    break;

  case file_mode::open_read:
    oflags = O_RDONLY;
    break;

  case file_mode::open_write:
    oflags = O_WRONLY;
    break;

  case file_mode::create_new:
    oflags = O_RDWR | O_CREAT;
    break;

  case file_mode::truncate:
       //Try to create
#ifndef _WIN32
          this->handle_ = ::open(filename.local_name().c_str(), O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
          if (0 > this->handle_) {
            auto error = errno;
            if(EEXIST != error) {
              throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
            }
          } else {
            return;
          }
#else
    this->handle_ = ::_open(this->filename_.local_name().c_str(), O_RDWR | O_CREAT | O_BINARY | O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
      if (0 > this->handle_) {
        auto error = GetLastError();
        if(EEXIST != error) {
          throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
        }
      } else {
        return;
      }
#endif
      oflags = O_RDWR | O_TRUNC;
    break;

  case file_mode::open_or_create:
    //Try to create
#ifndef _WIN32
      this->handle_ = ::open(filename.local_name().c_str(), O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
      if (0 > this->handle_) {
        auto error = errno;
        if(EEXIST != error) {
          throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
        }
      } else {
        return;
      }
#else
      this->handle_ = ::_open(this->filename_.local_name().c_str(), O_RDWR | O_CREAT | O_BINARY | O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
      if (0 > this->handle_) {
        auto error = GetLastError();
        if(EEXIST != error) {
          throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
        }
      } else {
        return;
      }
#endif
    oflags = O_RDWR;
    break;


  default:
    throw std::invalid_argument("Invalid mode for open file");
  }

#ifndef _WIN32
  this->handle_ = ::open(filename.local_name().c_str(), oflags, S_IREAD | S_IWRITE);
  if (0 > this->handle_) {
    auto error = errno;
    throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
  }
#else

  this->handle_ = ::_open(this->filename_.local_name().c_str(), oflags | O_BINARY | O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
  if (0 > this->handle_) {
    auto error = GetLastError();
    throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
  }
#endif
}


size_t vds::file::read(
  void * buffer,
  size_t buffer_len)
{
  auto readed = ::read(this->handle_, buffer, safe_cast<unsigned int>(buffer_len));
  if (0 > readed) {
#ifdef _WIN32
    auto error = GetLastError();
#else
    auto error = errno;
#endif
    throw std::system_error(error, std::system_category(), "Unable to read file " + this->filename_.str());
  }
  
  return (size_t)readed;
}

void vds::file::write(
  const void * buffer,
  size_t buffer_len)
{
  while (0 < buffer_len) {
    auto written = ::write(this->handle_, buffer, safe_cast<unsigned int>(buffer_len));
    if (0 > written) {
#ifdef _WIN32
      auto error = GetLastError();
#else
      auto error = errno;
#endif
      throw std::system_error(error, std::system_category(), "Unable to write file " + this->filename_.full_name());
    }

    if ((size_t)written == buffer_len) {
      return;
    }

    buffer_len -= written;
    buffer = (const uint8_t *)buffer + written;
  }
}

vds::file& vds::file::operator = (file&& f) {
  this->close();
  this->filename_ = std::move(f.filename_);
  this->handle_ = f.handle_;
  f.handle_ = 0;

  return *this;
}

vds::file vds::file::create_temp(const service_provider * sp) {
  foldername tmp_folder(persistence::current_user(sp), "tmp");
  tmp_folder.create();

  for (;;) {
    try {
      return file(filename(tmp_folder, std::to_string(std::rand()) + "." + std::to_string(std::rand()) + ".tmp"), file::file_mode::create_new);
    }
    catch(const std::system_error & ex) {
      if(EEXIST != ex.code().value()) {
        throw;
      }      
    }
  }
}

size_t vds::file::length() const
{
  struct stat buffer;
  if (0 != fstat(this->handle_, &buffer)) {
    auto error = errno;
    throw std::system_error(error, std::generic_category(), "Unable to get file size of " + this->filename_.full_name());
  }
  
  return buffer.st_size;
}

size_t vds::file::length(const filename & fn)
{
  struct stat buffer;
  if (0 != stat(fn.local_name().c_str(), &buffer)) {
    auto error = errno;
    throw std::system_error(error, std::generic_category(), "Unable to get file size of " + fn.name());
  }

  return buffer.st_size;
}

void vds::file::seek(size_t position)
{
  if (-1L == lseek(this->handle_, position, SEEK_SET)) {
    auto error = errno;
    throw std::system_error(error, std::generic_category(), "Unable to seek file position of " + this->filename_.full_name());
  }
}

bool vds::file::exists(const filename & fn)
{
  return (0 == access(fn.local_name().c_str(), 0));
}

void vds::file::flush()
{
#ifdef _WIN32
  HANDLE h = (HANDLE)_get_osfhandle(this->handle_);
  if (INVALID_HANDLE_VALUE == h) {
    auto err = GetLastError();
    throw std::system_error(err, std::generic_category(), "Unable to flush file " + this->filename_.full_name());
  }

  if (!FlushFileBuffers(h)) {
    auto err = GetLastError();
    throw std::system_error(err, std::system_category(), "Unable to flush file " + this->filename_.full_name());
  }

#else
  if (0 != ::fsync(this->handle_)) {
    auto error = errno;
    throw std::system_error(error, std::generic_category(), "Unable to flush file " + this->filename_.full_name());
  }
#endif
}

vds::output_text_stream::output_text_stream(file & f)
  : f_(f), written_(0)
{
}

vds::output_text_stream::~output_text_stream()
{
  this->flush();
}

void vds::output_text_stream::write(
  const std::string & value)
{
  const char * data = value.c_str();
  size_t len = value.length();
  
  if(sizeof(this->buffer_) < this->written_ + len) {
    if (0 < this->written_) {
      auto rest = sizeof(this->buffer_) - this->written_;
      if (rest > len) {
        rest = len;
      }

      if (rest > 0) {
        memcpy((uint8_t *)this->buffer_ + this->written_, data, rest);
        this->written_ += rest;
        this->f_.write(this->buffer_, this->written_);
        this->written_ = 0;
        len -= rest;
        data += rest;
      }
    }
    else {
      this->f_.write(data, len);
      len = 0;
    }
  }

  memcpy((uint8_t *)this->buffer_ + this->written_, data, len);
  this->written_ += len;
}

void vds::output_text_stream::flush()
{
  if (0 < this->written_) {
    this->f_.write(this->buffer_, this->written_);
    this->written_ = 0;
  }
}

vds::input_text_stream::input_text_stream(file & f)
  : f_(f), offset_(0), readed_(0)
{
}

bool vds::input_text_stream::read_line(std::string & result)
{
  result.clear();

  for (;;) {
    if (0 == this->readed_) {
      this->readed_ = this->f_.read(this->buffer_, sizeof(this->buffer_));
      if (0 == this->readed_) {
        return !result.empty();
      }

      this->offset_ = 0;
    }

    auto p = memchr(this->buffer_ + this->offset_, '\n', this->readed_);
    if (nullptr == p) {
      result.append(this->buffer_ + this->offset_, this->readed_);
      this->offset_ = 0;
      this->readed_ = 0;
    }
    else {
      auto len = (const char *)p - this->buffer_ - this->offset_;
      result.append(this->buffer_ - this->offset_, len - 1);

      this->offset_ += len;
      this->readed_ -= len;

      return true;
    }
  }
}

void vds::file::move(const vds::filename& source, const vds::filename& target)
{
  if(rename(source.local_name().c_str(), target.local_name().c_str())){
    auto error = errno;
    throw std::system_error(error, std::generic_category(), "Rename file " + source.full_name() + " to " + target.full_name());
  }
}

void vds::file::delete_file(const filename & fn, bool ignore_error /*= false*/)
{
  if (0 != remove(fn.local_name().c_str()) && !ignore_error) {
    auto err = errno;
    throw std::system_error(err, std::generic_category(), "Unable to delete file " + fn.full_name());
  }
}

std::string vds::file::read_all_text(const filename & fn)
{
  file f(fn, file::file_mode::open_read);

  std::string result;
  char buffer[1024];
  for (;;) {
    auto readed = f.read(buffer, sizeof(buffer));
    if (0 == readed) {
      break;
    }

    result.append(buffer, readed);
  }

  return result;
}

vds::const_data_buffer vds::file::read_all(const vds::filename& fn)
{
  file f(fn, file::file_mode::open_read);
  
  std::vector<uint8_t> buffer(f.length());
  
  f.read(buffer.data(), buffer.size());
  
  return const_data_buffer(buffer.data(), buffer.size());
}

void vds::file::write_all(const vds::filename &fn, const vds::const_data_buffer &data) {
  file f(fn, file::file_mode::truncate);
  f.write(data.data(), data.size());
  f.close();
}

