#ifndef __VDS_CORE_LOGGER_H_
#define __VDS_CORE_LOGGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <sstream>
#include "service_provider.h"

namespace vds {
    enum log_level {
        ll_trace,
        ll_debug,
        ll_info,
        ll_error
    };

    class log_record {
        public:
            log_record(log_level level)
            : level_(level) {
                switch (level) {
                    case ll_trace:
                        this->data_ << "TRACE: ";
                        break;
                        
                    case ll_debug:
                        this->data_ << "DEBUG: ";
                        break;
                    case ll_info:
                        this->data_ << "INFO: ";
                        break;

                    case ll_error:
                        this->data_ << "ERROR: ";
                        break;
                }
            }

            template <typename value_type>
            log_record & write(typename std::enable_if<!std::is_base_of<std::exception, value_type>::value, value_type>::type value)
            {
                this->data_ << value;
                return *this;
            }

            template <typename value_type>
            log_record & write(typename std::enable_if<std::is_base_of<std::exception, value_type>::value, value_type>::type value)
            {
                this->data_ << value.what();
                return *this;
            }

            template <typename value_type>
            log_record & operator << (value_type value)
            {
                return this->write<value_type>(value);
            }

            std::string str() const {
                return this->data_.str();
            }

        private:
            log_level level_;
            std::stringstream data_;
    };

    class trace : public log_record {
        public:
            trace(const char * text)
            : log_record(ll_trace)
            {
                if (nullptr != text) {
                    *this << text;
                }
            }
    };

    class debug : public log_record {
        public:
            debug(const char * text)
            : log_record(ll_debug)
            {
                if (nullptr != text) {
                    *this << text;
                }
            }
    };

    class info : public log_record {
        public:
            info(const char * text)
            : log_record(ll_info)
            {
                if (nullptr != text) {
                    *this << text;
                }
            }
    };

    class error : public log_record {
        public:
            error(const char * text)
            : log_record(ll_error)
            {
                if (nullptr != text) {
                    *this << text;
                }
            }
    };

    class log_writer
    {
    public:
        log_writer(std::function<void(const log_record &)> impl);
        void write(const log_record & record);

    private:
        std::function<void(const log_record &)> impl_;
    };

    class logger {
        public:
            logger(const service_provider & sp, const std::string & name);

            void operator () (const log_record & record);

        private:
            service_provider sp_;
            std::string name_;
            std::list<log_writer> log_writer_;
    };


    class console_logger : public iservice
    {
    public:
        void register_services(service_registrator &) override;
        void start(const service_provider &) override;
        void stop(const service_provider &) override;
    };
}

#endif//__VDS_CORE_LOGGER_H_
