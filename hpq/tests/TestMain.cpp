#include <gtest/gtest.h>

#include <QGuiApplication>

// Fonts need a QGuiApplication; run the tests without a display.
int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication application(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
