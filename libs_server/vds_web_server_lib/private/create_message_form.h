//#ifndef __VDS_WEB_SERVER_CREATE_MESSAGE_FORM_H__
//#define __VDS_WEB_SERVER_CREATE_MESSAGE_FORM_H__
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "http_form_parser.h"
//#include "user_manager.h"
//
//namespace vds {
//  class create_message_form : public http::form_parser {
//    using base_class = vds::http::form_parser;
//  public:
//    create_message_form(
//      const vds::service_provider* sp,
//      const std::shared_ptr<vds::user_manager>& user_mng);
//
//    vds::async_task<vds::expected<void>> on_field(const field_info& field) override;
//
//    vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> on_file(const file_info& file) override;
//
//    vds::async_task<vds::expected<void>> complete();
//
//  private:
//    std::shared_ptr<vds::user_manager> user_mng_;
//    vds::const_data_buffer channel_id_;
//    std::list<vds::transactions::user_message_transaction::file_info_t> files_;
//    std::shared_ptr<vds::json_object> message_;
//  };
//}
//
//#endif//__VDS_WEB_SERVER_CREATE_MESSAGE_FORM_H__