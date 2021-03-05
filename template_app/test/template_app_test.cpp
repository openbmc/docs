#include <template_app.hpp>

#include <gtest/gtest.h>

TEST(TemplateApp, changeIntegerValue)
{
    int currentValue = 0;
    // Good path.  Expect that changing the value to 42 should change it
    EXPECT_EQ(1, changeIntegerValue(42, currentValue));
    EXPECT_EQ(currentValue, 42);

    // Bad path, expecting that changing to 51 should return an error, and keep
    // the current value
    EXPECT_EQ(-EINVAL, changeIntegerValue(51, currentValue));
    EXPECT_EQ(currentValue, 42);
}

TEST(TemplateApp, unlockDoor)
{
    // Good path.  Expect that changing the value to 42 should change it
    EXPECT_EQ("DoorLocked", unlockDoor(""));
    EXPECT_EQ("DoorLocked", unlockDoor("BadKeyValue"));
    EXPECT_EQ("DoorUnlocked", unlockDoor("open sesame"));
}