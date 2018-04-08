#include "stdafx.h"
#include "private/file_upload_task_p.h"
#include "file_operations.h"

vds::async_task<> vds::_file_upload_task::read_part(const vds::service_provider &sp, const vds::http_message &part) {
  std::string content_disposition;
  if(part.get_header("Content-Disposition", content_disposition)){
    std::list<std::string> items;
    for(;;){
      auto p = content_disposition.find(';');
      if(std::string::npos == p){
        vds::trim(content_disposition);
        items.push_back(content_disposition);
        break;
      }
      else {
        items.push_back(vds::trim_copy(content_disposition.substr(0, p)));
        content_disposition.erase(0, p + 1);
      }
    }

    if(!items.empty() && "form-data" == *items.begin()){
      std::map<std::string, std::string> values;
      for(const auto & item : items){
        auto p = item.find('=');
        if(std::string::npos != p){
          auto value = item.substr(p + 1);
          if(!value.empty()
             && value[0] == '\"'
             && value[value.length() - 1] == '\"'){
            value.erase(0, 1);
            value.erase(value.length() - 1, 1);
          }

          values[item.substr(0, p)] = value;
        }
      }

      auto pname = values.find("name");
      if (values.end() != pname) {
        if ("channel_id" == pname->second) {
          auto buffer = std::make_shared<std::string>();
          return this->read_string_body(buffer, part).then([buffer, pthis = this->shared_from_this()]() {
            pthis->channel_id_ = base64::to_bytes(*buffer);
          });
        }
        else {
          return sp.get<file_manager::file_operations>()->upload_file(
            sp,
            this->user_mng_,
            this->channel_id_,
            pname->second,
            values["Content-Type"],
            part.body());
        }
      }
      return this->skip_part(part);
    }
  }

  return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
      .then([pthis = this->shared_from_this(), sp, part](size_t readed) -> vds::async_task<> {
        if(0 == readed) {
          return vds::async_task<>::empty();
        }

        return pthis->read_part(sp, part);
      });
}

vds::async_task<> vds::_file_upload_task::skip_part(const vds::http_message &part) {
  return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
      .then([pthis = this->shared_from_this(), part](size_t readed) -> vds::async_task<> {
        if(0 == readed) {
          return vds::async_task<>::empty();
        }

        return pthis->skip_part(part);
      });

}

vds::async_task<> vds::_file_upload_task::read_string_body(const std::shared_ptr<std::string> & buffer, const vds::http_message &part) {
  return part.body()->read_async(this->buffer_, sizeof(this->buffer_))
    .then([pthis = this->shared_from_this(), buffer, part](size_t readed)->vds::async_task<> {
    if (0 == readed) {
      return vds::async_task<>::empty();
    }
    *buffer += std::string((const char *)pthis->buffer_, readed);
    return pthis->read_string_body(buffer, part);
  });
}

vds::async_task<vds::http_message> vds::_file_upload_task::get_response(const vds::service_provider &sp) {
  return [pthis = this->shared_from_this()](const async_result<vds::http_message> & result){
    std::unique_lock<std::mutex> lock(pthis->result_mutex_);
    if(pthis->result_){
      result.done(pthis->result_);
      pthis->result_ = http_message();
    }
    else {
      pthis->result_done_ = result;
    }
  };
//  {
//    return vds::http_response::status_response(sp, vds::http_response::HTTP_OK, "OK");
//  }return vds::async_task<vds::http_message>(functor_type());
}
