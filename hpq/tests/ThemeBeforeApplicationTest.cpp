#include <hpq/ChinaInkTheme.h>
#include <hpq/NotebookTheme.h>
#include <hpq/SketchTheme.h>

#include <gtest/gtest.h>

#include <QApplication>

// A theme made before the QApplication: QFontDatabase cannot register the fonts of the themes
// yet. Its own executable, since the other tests run in a QApplication.

namespace {

TEST(ThemeBeforeApplicationTest, FontsAreRegisteredOnceTheApplicationExists) {
    // Before the application: no font is registered, and nothing crashes.
    EXPECT_TRUE(hpq::sketch_theme().font().family().isEmpty());
    EXPECT_TRUE(hpq::notebook_theme().font().family().isEmpty());
    EXPECT_TRUE(hpq::china_ink_theme().font().family().isEmpty());

    int argc = 1;
    char name[] = "HpqThemeBeforeApplicationTests";
    char *argv[] = {name, nullptr};
    QApplication application(argc, argv);

    EXPECT_EQ(hpq::sketch_theme().font().family(), "Architects Daughter");
    EXPECT_EQ(hpq::notebook_theme().font().family(), "Playwrite FR Moderne");
    EXPECT_EQ(hpq::china_ink_theme().font().family(), "Kaushan Script");
}

}  // namespace

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
