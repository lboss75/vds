/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "version.h"
#include "string_format.h"

vds::version vds::version::parse(std::string value) {
    version result = { -1, -1, -1 };
    sscanf(value,"%d.%d.%d.%d",&result.major,&result.minor,&result.build,&result.revision);
    return result;
}

std::string vds::version::to_string() {
    if(this->major < 0){
        return "0";
    }

    if(this->minor < 0){
        return string_format("%d", this->major);
    }

    if(this->build < 0){
        return string_format("%d.%d", this->major, this->minor);
    }

    if(this->revision < 0){
        return string_format("%d.%d.%d", this->major, this->minor, this->build);
    }

    return string_format("%d.%d.%d.%d", this->major, this->minor, this->build, this->revision);
}
