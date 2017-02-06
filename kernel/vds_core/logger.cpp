/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "logger.h"

vds::logger::logger(const service_provider & sp, const std::string & name)
: sp_(sp), name_(name), log_writer_(sp.get_collection<log_writer>())
{
}

void vds::logger::operator()(const log_record & record) const
{
    for (auto & p : this->log_writer_) {
        p.write(record);
    }
}
/////////////////////////////////////////////////////////
vds::log_writer::log_writer(std::function<void(const log_record&)> impl)
    : impl_(impl)
{
}

void vds::log_writer::write(const log_record & record) const
{
    this->impl_(record);
}
/////////////////////////////////////////////////////////
void vds::console_logger::register_services(service_registrator & registrator)
{
    registrator.add_collection_factory<log_writer>([]()->log_writer {
        return log_writer([](const log_record & record) {
            std::cout << record.str() << '\n';
        });
    });
}

void vds::console_logger::start(const service_provider &)
{
}

void vds::console_logger::stop(const service_provider &)
{
}

