#include "stdafx.h"
#include "file.h"

vds::file::file(const filename & filename, file_mode mode)
  : filename_(filename)
{
  int oflags;
  switch (mode) {
  case open_read:
    oflags = O_RDONLY;
    break;

  case open_write:
    oflags = O_WRONLY;
    break;

  case create_new:
    oflags = O_WRONLY | O_CREAT | O_EXCL;
    break;

  case truncate:
    oflags = O_WRONLY | O_CREAT | O_TRUNC;
    break;

  default:
    throw new std::invalid_argument("Invalid mode for open file");
  }

#ifndef _WIN32
  this->handle_ = open(filename.local_name().c_str(), oflags, S_IREAD | S_IWRITE);
  if (0 > this->handle_) {
    auto error = errno;
    throw new std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
  }
#else

  this->handle_ = _open(this->filename_.local_name().c_str(), oflags | O_BINARY | O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
  if (0 > this->handle_) {
    auto error = GetLastError();
    throw new std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
  }
#endif
}

vds::file::~file()
{
#ifndef _WIN32
  close(this->handle_);
#else
  _close(this->handle_);
#endif
}

size_t vds::file::read(void * buffer, size_t buffer_len)
{
  auto readed = ::read(this->handle_, buffer, buffer_len);
  if (0 > readed) {
#ifdef _WIN32
    auto error = GetLastError();
#else
    auto error = errno;
#endif
    throw new std::system_error(error, std::system_category(), "Unable to read file " + this->filename_.str());
  }

  return (size_t)readed;
}

void vds::file::write(const void * buffer, size_t buffer_len)
{
  while (0 < buffer_len) {
    auto written = ::write(this->handle_, buffer, buffer_len);
    if (0 > written) {
#ifdef _WIN32
      auto error = GetLastError();
#else
      auto error = errno;
#endif
      throw new std::system_error(error, std::system_category(), "Unable to write file " + this->filename_.str());
    }

    if ((size_t)written == buffer_len) {
      return;
    }

    buffer_len -= written;
    buffer = (const uint8_t *)buffer + written;
  }
}

vds::output_text_stream::output_text_stream(file & f)
  : f_(f), written_(0)
{
}

vds::output_text_stream::~output_text_stream()
{
  this->flush();
}

void vds::output_text_stream::write(const std::string & value)
{
  const char * data = value.c_str();
  size_t len = value.length();
  
  while (sizeof(this->buffer_) < this->written_ + len) {
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
