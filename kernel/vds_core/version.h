#ifndef __VDS_CORE_VDS_VERSION_H_
#define __VDS_CORE_VDS_VERSION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
    struct version {
        int major_version;
        int minor_version;
        int build_version;
        int revision_version;

        version(int major_, int minor_)
        : major_version(major_), minor_version(minor_), build_version(-1), revision_version(-1) {
        }

        version(int major_, int minor_, int build_)
        : major_version(major_), minor_version(minor_), build_version(build_), revision_version(-1) {
        }
        version(int major_, int minor_, int build_, int revision_)
        : major_version(major_), minor_version(minor_), build_version(build_), revision_version(revision_) {
        }

        static version parse(std::string value);
        std::string to_string() const;
    };
};


#endif //__VDS_CORE_VDS_VERSION_H_
