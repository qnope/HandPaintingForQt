#include <hpq/Hpq.h>
#include <hpq/Version.h>

#include <gtest/gtest.h>

#include <string>

TEST(Version, StringIsMajorMinorPatch) {
    const std::string expected = std::to_string(HPQ_VERSION_MAJOR) + "." + std::to_string(HPQ_VERSION_MINOR) +
                                 "." + std::to_string(HPQ_VERSION_PATCH);
    EXPECT_EQ(expected, HPQ_VERSION_STRING);
}

TEST(Version, LibraryHasTheVersionOfItsHeaders) { EXPECT_STREQ(HPQ_VERSION_STRING, hpq::version()); }
