#include "AppFont.h"

#include <QApplication>
#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    hpq::example::apply_default_font();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
