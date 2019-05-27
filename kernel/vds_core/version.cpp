/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "version.h"
#include "string_format.h"

vds::version vds::version::parse(std::string value) {
    version result = { -1, -1, -1 };
    sscanf(value.c_str(),"%d.%d.%d.%d",&result.major_version,&result.minor_version,&result.build_version,&result.revision_version);
    return result;
}

std::string vds::version::to_string() const {
    if(this->major_version < 0){
        return "0";
    }

    if(this->minor_version < 0){
        return string_format("%d", this->major_version);
    }

    if(this->build_version < 0){
        return string_format("%d.%d", this->major_version, this->minor_version);
    }

    if(this->revision_version < 0){
        return string_format("%d.%d.%d", this->major_version, this->minor_version, this->build_version);
    }

    return string_format("%d.%d.%d.%d", this->major_version, this->minor_version, this->build_version, this->revision_version);
}
