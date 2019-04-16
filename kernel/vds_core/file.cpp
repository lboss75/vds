#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include <sys/stat.h>
#include "file.h"
#include "persistence.h"

vds::file::file()
: handle_(0)
{
}


vds::file::file(file&& f) noexcept 
: filename_(f.filename_), handle_(f.handle_){
  f.handle_ = 0;
}

vds::file::~file()
{
  (void)this->close();
}

vds::expected<void> vds::file::close()
{
  if(0 < this->handle_){
    int error = 0;
#ifndef _WIN32
    if(0 > ::close(this->handle_)) {
      error = errno;
    }
#else
    if (0 > ::_close(this->handle_)) {
      error = errno;
    }
#endif
    this->handle_ = 0;

    if (error != 0) {
      return vds::make_unexpected<std::system_error>(error, std::system_category(), "Unable to close file " + this->filename_.str());
    }
  }

  return expected<void>();
}
  
vds::expected<void> vds::file::open(const vds::filename& filename, vds::file::file_mode mode)
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
    oflags = O_RDWR | O_CREAT | O_EXCL;
    break;

  case file_mode::truncate:
       //Try to create
      oflags = O_RDWR | O_CREAT | O_TRUNC;
    break;

  case file_mode::open_or_create:
    oflags = O_RDWR | O_CREAT;
    break;


  default:
    return vds::make_unexpected<std::invalid_argument>("Invalid mode for open file");
  }

#ifndef _WIN32
  this->handle_ = ::open(filename.local_name().c_str(), oflags, S_IREAD | S_IWRITE);
  if (0 > this->handle_) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Unable to open file " + this->filename_.str());
  }
#else

  this->handle_ = ::_open(this->filename_.local_name().c_str(), oflags | O_BINARY | O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
  if (0 > this->handle_) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Unable to open file " + this->filename_.str());
  }
#endif
  return expected<void>();
}


vds::expected<size_t> vds::file::read(
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
    return vds::make_unexpected<std::system_error>(error, std::system_category(), "Unable to read file " + this->filename_.str());
  }
  
  return (size_t)readed;
}

vds::expected<void> vds::file::write(
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
      return vds::make_unexpected<std::system_error>(error, std::system_category(), "Unable to write file " + this->filename_.full_name());
    }

    if ((size_t)written == buffer_len) {
      return expected<void>();
    }

    buffer_len -= written;
    buffer = (const uint8_t *)buffer + written;
  }

  return expected<void>();
}

vds::file& vds::file::operator = (file&& f) noexcept {
  (void)this->close();
  this->filename_ = std::move(f.filename_);
  this->handle_ = f.handle_;
  f.handle_ = 0;

  return *this;
}

vds::expected<vds::file> vds::file::create_temp(const service_provider * sp) {
  GET_EXPECTED(pf, persistence::current_user(sp));
  foldername tmp_folder(pf, "tmp");
  CHECK_EXPECTED(tmp_folder.create());

  for (;;) {
    file f;
    auto r = f.open(filename(tmp_folder, std::to_string(std::rand()) + "." + std::to_string(std::rand()) + ".tmp"), file::file_mode::create_new);
    if(r.has_value()) {
      return std::move(f);
    }

    const auto sys_error = dynamic_cast<const std::system_error *>(r.error().get());
    if(nullptr == sys_error || sys_error->code().value() != EEXIST) {
      return unexpected(std::move(r.error()));
    }
  }
}

vds::expected<size_t> vds::file::length() const
{
  struct stat buffer;
  if (0 != fstat(this->handle_, &buffer)) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category(), "Unable to get file size of " + this->filename_.full_name());
  }
  
  return buffer.st_size;
}

vds::expected<size_t> vds::file::length(const filename & fn)
{
  struct stat buffer;
  if (0 != stat(fn.local_name().c_str(), &buffer)) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category(), "Unable to get file size of " + fn.name());
  }

  return buffer.st_size;
}

vds::expected<void> vds::file::seek(size_t position)
{
  if (-1L == lseek(this->handle_, position, SEEK_SET)) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category(), "Unable to seek file position of " + this->filename_.full_name());
  }

  return expected<void>();
}

bool vds::file::exists(const filename & fn)
{
  return (0 == access(fn.local_name().c_str(), 0));
}

vds::expected<void> vds::file::flush()
{
#ifdef _WIN32
  HANDLE h = (HANDLE)_get_osfhandle(this->handle_);
  if (INVALID_HANDLE_VALUE == h) {
    auto err = GetLastError();
    return vds::make_unexpected<std::system_error>(err, std::generic_category(), "Unable to flush file " + this->filename_.full_name());
  }

  if (!FlushFileBuffers(h)) {
    auto err = GetLastError();
    return vds::make_unexpected<std::system_error>(err, std::system_category(), "Unable to flush file " + this->filename_.full_name());
  }

#else
  if (0 != ::fsync(this->handle_)) {
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category(), "Unable to flush file " + this->filename_.full_name());
  }
#endif
  return vds::expected<void>();
}

vds::expected<void> vds::file::copy(const filename& source, const filename& target, bool override_exist) {
  file fin;
  file fout;
  CHECK_EXPECTED(fin.open(source, file_mode::open_read));
  CHECK_EXPECTED(fout.open(target, override_exist ? file_mode::open_or_create : file_mode::create_new));

  uint8_t buffer[1024];
  for(;;) {
    GET_EXPECTED(readed, fin.read(buffer, sizeof(buffer)));
    if(0 == readed) {
      break;
    }

    CHECK_EXPECTED(fout.write(buffer, readed));
  }

  CHECK_EXPECTED(fin.close());
  CHECK_EXPECTED(fout.close());

  return expected<void>();
}

vds::output_text_stream::output_text_stream(file & f)
  : f_(f), written_(0)
{
}

vds::output_text_stream::~output_text_stream()
{
  (void)this->flush();
}

vds::expected<void> vds::output_text_stream::write(
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
        CHECK_EXPECTED(this->f_.write(this->buffer_, this->written_));
        this->written_ = 0;
        len -= rest;
        data += rest;
      }
    }
    else {
      CHECK_EXPECTED(this->f_.write(data, len));
      len = 0;
    }
  }

  memcpy((uint8_t *)this->buffer_ + this->written_, data, len);
  this->written_ += len;

  return expected<void>();
}

vds::expected<void> vds::output_text_stream::flush()
{
  if (0 < this->written_) {
    CHECK_EXPECTED(this->f_.write(this->buffer_, this->written_));
    this->written_ = 0;
  }

  return expected<void>();
}

vds::input_text_stream::input_text_stream(file & f)
  : f_(f), offset_(0), readed_(0)
{
}

vds::expected<bool> vds::input_text_stream::read_line(std::string & result)
{
  result.clear();

  for (;;) {
    if (0 == this->readed_) {
      GET_EXPECTED(readed, this->f_.read(this->buffer_, sizeof(this->buffer_)));

      this->readed_ = readed;
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

vds::expected<void> vds::file::move(const vds::filename& source, const vds::filename& target)
{
  if(rename(source.local_name().c_str(), target.local_name().c_str())){
    auto error = errno;
    return vds::make_unexpected<std::system_error>(error, std::generic_category(), "Rename file " + source.full_name() + " to " + target.full_name());
  }

  return expected<void>();
}

vds::expected<void> vds::file::delete_file(const filename & fn)
{
  if (0 != remove(fn.local_name().c_str())) {
    auto err = errno;
    return vds::make_unexpected<std::system_error>(err, std::generic_category(), "Unable to delete file " + fn.full_name());
  }

  return expected<void>();
}

vds::expected<std::string> vds::file::read_all_text(const filename & fn)
{
  file f;
  CHECK_EXPECTED(f.open(fn, file::file_mode::open_read));

  std::string result;
  char buffer[1024];
  for (;;) {
    GET_EXPECTED(readed, f.read(buffer, sizeof(buffer)));
    if (0 == readed) {
      break;
    }

    result.append(buffer, readed);
  }

  return result;
}

vds::expected<vds::const_data_buffer> vds::file::read_all(const vds::filename& fn)
{
  file f;
  CHECK_EXPECTED(f.open(fn, file::file_mode::open_read));

  GET_EXPECTED(len, f.length());
  std::vector<uint8_t> buffer(len);
  
  CHECK_EXPECTED(f.read(buffer.data(), buffer.size()));
  
  return const_data_buffer(buffer.data(), buffer.size());
}

vds::expected<void> vds::file::write_all(const vds::filename &fn, const vds::const_data_buffer &data) {
  file f;
  CHECK_EXPECTED(f.open(fn, file::file_mode::truncate));
  CHECK_EXPECTED(f.write(data.data(), data.size()));
  CHECK_EXPECTED(f.close());

  return expected<void>();
}

