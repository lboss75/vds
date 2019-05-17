//#ifndef __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_
//#define __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "http_message.h"
//
//namespace vds {
//  namespace http {
//    class simple_form_parser : public std::enable_shared_from_this<simple_form_parser> {
//    public:
//      static async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> parse(
//        const service_provider * sp,
//        const http_message& message,
//        lambda_holder_t<async_task<expected<void>>, const std::shared_ptr<http::simple_form_parser> &> handler);
//
//
//
//      const std::map<std::string, std::string> & values() const {
//        return this->values_;
//      }
//
//    private:
//      std::map<std::string, std::string> values_;
//
//      static vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> read_part(
//        const std::shared_ptr<simple_form_parser> & owner,
//        const http_message & part);
//
//      static vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> read_form_urlencoded(
//        const std::shared_ptr<simple_form_parser> & owner,
//        const http_message & part,
//        lambda_holder_t<vds::async_task<vds::expected<void>>> final_handler);
//
//
//      static vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> read_string_body(
//        lambda_holder_t<async_task<expected<void>>, std::string &&> handler);
//    };
//  }
//}
//
//#endif // __VDS_HTTP_HTTP_SIMPLE_FORM_PARSER_H_
//
