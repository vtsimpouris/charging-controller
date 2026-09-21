#include <gtest/gtest.h>

#include "CurrentController.hpp"


TEST(CurrentControllerTest, ProducesPositiveControlForPositiveError)
{
    CurrentController controller(2.0f);

    const float output =
        controller.update(120.0f, 100.0f);

    EXPECT_FLOAT_EQ(output, 40.0f);
}


TEST(CurrentControllerTest, ProducesNegativeControlForNegativeError)
{
    CurrentController controller(2.0f);

    const float output =
        controller.update(120.0f, 130.0f);

    EXPECT_FLOAT_EQ(output, -20.0f);
}


TEST(CurrentControllerTest, ProducesZeroControlAtReference)
{
    CurrentController controller(2.0f);

    const float output =
        controller.update(120.0f, 120.0f);

    EXPECT_FLOAT_EQ(output, 0.0f);
}