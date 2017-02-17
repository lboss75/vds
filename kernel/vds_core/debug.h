#ifndef __VDS_CORE_DEBUG_H_
#define __VDS_CORE_DEBUG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class lifetime_check {
  public:
    lifetime_check() {
      this->signature_[0] = 'L';
      this->signature_[1] = 'I';
      this->signature_[2] = 'F';
      this->signature_[3] = 'E';
    }

    ~lifetime_check()
    {
      this->signature_[0] = 'D';
      this->signature_[1] = 'I';
      this->signature_[2] = 'E';
      this->signature_[3] = 'D';
    }

    void validate() const {
      if (this->signature_[0] != 'L'
        || this->signature_[1] != 'I'
        || this->signature_[2] != 'F'
        || this->signature_[3] != 'E') {
        throw new std::runtime_error("Object disposed");
      }
    }

  private:
    char signature_[4];
  };
}



#endif//__VDS_CORE_DEBUG_H_
