#include "stdafx.h"
#include "test_config.h"

int main(int argc, char **argv) {

    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);

    test_config::instance().parse(argc, argv);

    return RUN_ALL_TESTS();
}

