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

        bool operator < (const version & r) const {
          if(this->major_version != r.major_version) {
            return (this->major_version < r.major_version);
          }
          if (this->minor_version != r.minor_version) {
            return (this->minor_version < r.minor_version);
          }
          if (this->build_version != r.build_version) {
            return (this->build_version < r.build_version);
          }
          return (this->revision_version < r.revision_version);
        }

        bool operator > (const version & r) const {
          if (this->major_version != r.major_version) {
            return (this->major_version > r.major_version);
          }
          if (this->minor_version != r.minor_version) {
            return (this->minor_version > r.minor_version);
          }
          if (this->build_version != r.build_version) {
            return (this->build_version > r.build_version);
          }
          return (this->revision_version > r.revision_version);
        }

        bool operator <= (const version & r) const {
          if (this->major_version != r.major_version) {
            return (this->major_version < r.major_version);
          }
          if (this->minor_version != r.minor_version) {
            return (this->minor_version < r.minor_version);
          }
          if (this->build_version != r.build_version) {
            return (this->build_version < r.build_version);
          }
          return (this->revision_version <= r.revision_version);
        }

        bool operator >= (const version & r) const {
          if (this->major_version != r.major_version) {
            return (this->major_version > r.major_version);
          }
          if (this->minor_version != r.minor_version) {
            return (this->minor_version > r.minor_version);
          }
          if (this->build_version != r.build_version) {
            return (this->build_version > r.build_version);
          }
          return (this->revision_version >= r.revision_version);
        }
    };
};


#endif //__VDS_CORE_VDS_VERSION_H_
