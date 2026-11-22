#include <gtest/gtest.h>

#include <hpq/Hpq.h>

#include <string>

TEST(Hpq, LibraryNameComesFromNamespaceHpq) { EXPECT_EQ(std::string("hpq"), hpq::library_name()); }
