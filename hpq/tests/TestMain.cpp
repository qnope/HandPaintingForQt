#include <gtest/gtest.h>

#include <QApplication>

// Fonts and widgets need a QApplication; run the tests without a display.
int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication application(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
