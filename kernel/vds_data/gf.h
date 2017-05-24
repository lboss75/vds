#ifndef __VDS_DATA_GF_H_
#define __VDS_DATA_GF_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <unordered_map>
#include <string>
#include <assert.h>

namespace vds {
  template <unsigned int m>
  class gf
  {
    constexpr static uint8_t hi_bit_mask = (1 << ((m - 1) % 8));
    constexpr static size_t hi_bit_index = (m - 1) / 8;
  public:
    constexpr static size_t ArraySize = (m + 7) / 8;
    typedef uint8_t DataType[(m + 7) / 8];

    gf(const gf & left, const gf & right) {
      for (size_t i = 0; i < sizeof(this->data_); ++i) {
        this->data_[i] = left.data_[i] ^ right.data_[i];
      }
    }

    gf(const DataType & data) {
      copy(this->data_, data);
    }

    ~gf() {

    }

    gf operator + (const gf & right) const {
      return gf(*this, right);
    }

    gf operator * (const gf & right) const {
      uint8_t a[ArraySize];
      uint8_t b[ArraySize];
      copy(a, this->data_);
      copy(b, right.data_);

      uint8_t data[ArraySize];
      memset(data, 0, sizeof(data));

      //
      for (size_t i = 0; i < m; ++i) {
        if (0 != (b[0] & 1)) {
          exclude_or(data, a);
        }

        bool hi_bit_set = (0 != (hi_bit_mask & a[hi_bit_index]));
        shift_left(a);
        if (hi_bit_set) {
          exclude_or(a, get_polynomial());
        }

        shift_right(b);
      }

      return gf(data);
    }

    bool operator == (const gf & right) const {
      return 0 == memcmp(this->data_, right.data_, sizeof(data_));
    }

    const DataType & data() const {
      return this->data_;
    }

    std::string toString() const {
      std::string result;
      for (size_t i = 0; i < ArraySize; ++i) {
        char buffer[20];
        sprintf(buffer, "%02x", this->data_[i]);
        result += buffer;
      }

      return result;
    }

  private:
    DataType data_;

    static void copy(DataType & a, const DataType & b);
    static void exclude_or(DataType & a, const DataType & b);
    static void shift_left(DataType & a);
    static void shift_right(DataType & a);

    static const DataType & get_polynomial();
  };


  template <>
  inline const gf<3>::DataType & gf<3>::get_polynomial()
  {
    static uint8_t result[1] = { 0x0b }; /* x^3 + x + 1 */
    return result;
  }

  template <>
  inline const gf<8>::DataType & gf<8>::get_polynomial()
  {
    static uint8_t result[1] = { 0x1D }; /* x^8 + x^4 + x^3 + x^2 + 1 */
    return result;
  }

  template <>
  inline const gf<16>::DataType & gf<16>::get_polynomial()
  {
    static uint8_t result[2] = { 0x0B, 0x10 };
    return result;
  }

  template <>
  inline const gf<32>::DataType & gf<32>::get_polynomial()
  {
    static uint8_t result[4] = { 0x07, 0x00, 0x40, 0x00 }; 
    return result;
  }

  template <typename value_type>
  class gf_math;

  template <>
  class gf_math<uint8_t>
  {
  public:
    gf_math() {
      memset(this->value2log_, 0, sizeof(this->value2log_));
      //uint8_t one[1] = { 1 };
      uint8_t two[1] = { 2 };

      this->log2value_[0] = 1;
      this->value2log_[1] = 0;

      gf<8> k(two);
      gf<8> v(two);
      for (uint8_t log = 1; log < 255; ++log) {
        uint8_t value = k.data()[0];

        this->log2value_[log] = value;
        assert(0 == this->value2log_[value]);
        this->value2log_[value] = log;

        k = k * v;
      }
    }

    uint8_t mul(uint8_t left, uint8_t right) const {
      if (left == 0 || right == 0) {
        return 0;
      }

      auto log_left = this->value2log_[left];
      auto log_right = this->value2log_[right];

      return this->log2value_[(log_left + log_right) % 255];
    }

    uint8_t div(uint8_t left, uint8_t right) const {
      if (left == 0 || right == 0) {
        return 0;
      }

      int dif = (int)this->value2log_[left] - (int)this->value2log_[right];
      while (dif < 0) {
        dif += 255;
      }


      return this->log2value_[dif % 255];
    }

    uint8_t add(uint8_t left, uint8_t right) const {
      return left ^ right;
    }

    uint8_t sub(uint8_t left, uint8_t right) const {
      return left ^ right;
    }
  private:
    uint8_t value2log_[0x100];
    uint8_t log2value_[0x100];
  };
  
  template <>
  class gf_math<uint16_t>
  {
  public:
    gf_math() {
      memset(this->value2log_, 0, sizeof(this->value2log_));
      
      uint8_t two[2] = { 2, 0 };

      this->log2value_[0] = 1;
      this->value2log_[1] = 0;

      gf<16> k(two);
      gf<16> v(two);
      for (uint16_t log = 1; log < 0xFFFF; ++log) {
        uint16_t value = (k.data()[1] << 8) | k.data()[0];

        this->log2value_[log] = value;
        assert(0 == this->value2log_[value]);
        this->value2log_[value] = log;

        k = k * v;
      }
    }

    uint16_t mul(uint16_t left, uint16_t right) const {
      if (left == 0 || right == 0) {
        return 0;
      }

      auto log_left = this->value2log_[left];
      auto log_right = this->value2log_[right];

      return this->log2value_[(log_left + log_right) % 0xFFFF];
    }

    uint16_t div(uint16_t left, uint16_t right) const {
      if (left == 0 || right == 0) {
        return 0;
      }

      int dif = (int)this->value2log_[left] - (int)this->value2log_[right];
      while (dif < 0) {
        dif += 0xFFFF;
      }


      return this->log2value_[dif % 0xFFFF];
    }

    uint16_t add(uint16_t left, uint16_t right) const {
      return left ^ right;
    }

    uint16_t sub(uint16_t left, uint16_t right) const {
      return left ^ right;
    }
  private:
    uint16_t value2log_[0x10000];
    uint16_t log2value_[0x10000];
  };

  template<unsigned int m>
  inline void gf<m>::copy(DataType & a, const DataType & b)
  {
    memcpy(&a, &b, sizeof(DataType));
  }

  template<unsigned int m>
  inline void gf<m>::exclude_or(DataType & a, const DataType & b)
  {
    for (size_t i = 0; i < ArraySize; ++i) {
      a[i] ^= b[i];
    }
  }
  template<unsigned int m>
  inline void gf<m>::shift_left(DataType & a)
  {
    bool save_hi_bit = false;
    for (size_t i = 0; i < ArraySize; ++i) {
      bool hi_bit = (0 != (0x80 & a[i]));
      a[i] <<= 1;
      if (save_hi_bit) {
        a[i] |= 1;
      }

      save_hi_bit = hi_bit;
    }
  }
  template<unsigned int m>
  inline void gf<m>::shift_right(DataType & a)
  {
    bool save_lo_bit = false;
    for (size_t i = ArraySize; i > 0; --i) {
      bool lo_bit = (0 != (1 & a[i - 1]));
      a[i - 1] >>= 1;
      if (save_lo_bit) {
        a[i - 1] |= 0x80;
      }

      save_lo_bit = lo_bit;
    }
  }
}

#endif//__VDS_DATA_GF_H_