#include <hpq/HandPaintStyle.h>

#include <gtest/gtest.h>

#include <QProxyStyle>
#include <QString>

namespace {

const QString FusionStyleName("fusion");

}  // namespace

TEST(HandPaintStyleTest, IsAProxyOverTheFusionStyle) {
    hpq::HandPaintStyle style;

    const QProxyStyle *proxy = qobject_cast<const QProxyStyle *>(&style);
    ASSERT_NE(proxy, nullptr);
    ASSERT_NE(proxy->baseStyle(), nullptr);
    EXPECT_EQ(proxy->baseStyle()->name().compare(FusionStyleName, Qt::CaseInsensitive), 0);
}
