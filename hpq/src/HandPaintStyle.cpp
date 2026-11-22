#include <hpq/HandPaintStyle.h>

#include <QStyleFactory>

namespace hpq {

namespace {

const QString BaseStyleName("Fusion");

}  // namespace

// QProxyStyle takes the ownership of the base style.
HandPaintStyle::HandPaintStyle() : QProxyStyle(QStyleFactory::create(BaseStyleName)) {}

}  // namespace hpq
