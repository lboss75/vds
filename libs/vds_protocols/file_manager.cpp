/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "file_manager.h"
#include "file_manager_p.h"
#include "connection_manager.h"

vds::file_manager::file_manager(_file_manager * impl)
: impl_(impl)
{
}

vds::async_task<>
vds::file_manager::put_file(
  const std::string & version_id,
  const std::string & user_login,
  const std::string & name,
  const filename & fn)
{
  return this->impl_->put_file(version_id, user_login, name, fn);
}

vds::async_task<const vds::filename&> vds::file_manager::download_file(
  const std::string & user_login,
  const std::string & name)
{
  return this->impl_->download_file(user_login, name);
}

vds::async_task<const vds::filename&> vds::file_manager::download_file(
  const guid & server_id,
  const std::string & version_id)
{
  return this->impl_->download_file(server_id, version_id);
}

///////////////////////////////////////////////////////////////////////////
vds::_file_manager::_file_manager(const service_provider & sp)
  : sp_(sp)
{
}


vds::async_task<>
vds::_file_manager::put_file(
  const std::string & version_id,
  const std::string & user_login,
  const std::string & name,
  const filename & fn)
{
  auto result = std::make_shared<server_log_file_map>(version_id, user_login, name);

  return create_async_task(
    [this, fn, result](const std::function<void(void)> & done, const error_handler & on_error) {
    dataflow(
      read_file(fn, (size_t)5 * 1024 * 1024),
      task_step([this, result](
        const std::function<void(void) > & done,
        const error_handler & on_error,
        const std::function<void(void)> & prev,
        const void * data, size_t size) {
      if (0 == size) {
        done();
        return;
      }

      this->chunk_manager_.get(this->sp_).add(const_data_buffer(data, size)).wait(
        [prev, result](const server_log_new_object & index) {
        result->add(index);
        prev();
      },
        on_error);
    }))
        (
          [this, done, result]() {
            this->storage_log_.get(this->sp_).add_to_local_log(result->serialize().get());
            done();
          },
          on_error);
  });
}

//vds::async_task<> vds::_storage_log::save_file(
//  const std::string & version_id,
//  const std::string & user_login,
//  const std::string & name,
//  const filename & tmp_file)
//{
//  return this->chunk_manager_
//    .get(this->sp_)
//    .add(version_id, user_login, name, tmp_file)
//    .then([this](
//      const std::function<void(void)> & done,
//      const error_handler & on_error,
//      const server_log_file_map & fm) {
//    this->add_to_local_log(fm.serialize().get());
//    done();
//  });

vds::async_task<const vds::filename&> vds::_file_manager::download_file(
  const std::string & user_login,
  const std::string & name)
{
  std::list<server_log_file_version> versions;
  this->db_.get(this->sp_).get_file_versions(user_login, name, versions);

  if (versions.empty()) {
    throw std::runtime_error("File not found");
  }

  auto top_version = *versions.begin();
  return this->download_file(top_version.server_id(), top_version.version_id());
}


class get_file_results : public std::enable_shared_from_this<get_file_results>
{
public:
  get_file_results(size_t count)
    : count_(count)
  {
  }

  void get_object_done(uint64_t index, const vds::filename & fn)
  {
    std::lock_guard<std::mutex> lock(this->data_mutex_);
    this->results_[index] = fn;

    this->try_complete();
  }

  void get_object_error(std::exception_ptr ex)
  {
    std::lock_guard<std::mutex> lock(this->data_mutex_);
    this->errors_.push_back(ex);

    this->try_complete();
  }

  void wait(
    const std::function<void(std::map<uint64_t, vds::filename> & result)> & done,
    const vds::error_handler & on_error)
  {
    std::lock_guard<std::mutex> lock(this->data_mutex_);
    this->done_ = done;
    this->on_error_ = on_error;

    this->try_complete();
  }

private:
  const size_t count_;

  std::mutex data_mutex_;

  std::map<uint64_t, vds::filename> results_;
  std::list<std::exception_ptr> errors_;

  std::function<void(std::map<uint64_t, vds::filename> & result)> done_;
  vds::error_handler on_error_;

  void try_complete()
  {
    if (this->count_ == this->results_.size() + this->errors_.size()) {
      if (!this->done_) {
        return;
      }

      if (!this->errors_.empty()) {
        this->on_error_(*this->errors_.begin());
      }
      else {
        this->done_(this->results_);
      }
    }
  }
};


vds::async_task<const vds::filename&> vds::_file_manager::download_file(
  const guid & server_id,
  const std::string & version_id)
{
  std::list<uint64_t> indexes;
  this->db_.get(this->sp_).get_file_version_map(server_id, version_id, indexes);

  if (indexes.empty()) {
    throw std::runtime_error("File '" + version_id + "' not found");
  }

  auto result = std::make_shared<get_file_results>(indexes.size());
  for (auto index : indexes) {
    auto fn = this->cache_.get(this->sp_).get_object_filename(server_id, index);

    if (file::exists(fn)) {
      result->get_object_done(index, fn);
    }
    else {
      this->connection_manager_.get(this->sp_)
        .download_object(server_id, index, fn)
        .wait(
          [result, index, fn]() { result->get_object_done(index, fn); },
          [result](std::exception_ptr ex) {result->get_object_error(ex); }
      );
    }
  }

  return create_async_task([this, result, indexes, version_id](
    const std::function<void(const vds::filename &)> & done,
    const error_handler & on_error) {
    result->wait(
      [this, done, on_error, indexes, version_id](std::map<uint64_t, vds::filename> & result) {
        foldername tmp(persistence::current_user(this->sp_), "tmp");
        tmp.create();

        filename tmpfile(tmp, version_id);
        file f(tmpfile, file::create_new);

        for (auto index : indexes) {
          barrier b;
          dataflow(
            read_file(result[index]),
            inflate(),
            append_to_file(f))(
              [&b]() { b.set(); },
              [&b, on_error](std::exception_ptr ex) { b.set(); on_error(ex); });

          b.wait();
        }

        f.close();

        done(tmpfile);
      },
      on_error);
  });
}

