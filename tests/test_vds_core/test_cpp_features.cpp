/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "test_cpp_features.h"

void throw_system_ex(){
    throw std::system_error(EFAULT, std::system_category(), "Test");
}

TEST(test_cpp_feature, test_exception) {
    try {
        throw_system_ex();
    }
    catch(const std::system_error & ex){
        GTEST_ASSERT_EQ(ex.code().category(), std::system_category());
        GTEST_ASSERT_EQ(ex.code().value(), EFAULT);
    }
}

