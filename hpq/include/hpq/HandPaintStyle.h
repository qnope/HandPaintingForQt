#pragma once

#include <hpq/FwdDecls.h>

#include <QProxyStyle>

namespace hpq {

// Fusion, with the parts Fusion draws as images drawn with flat vector primitives instead,
// so that a HandPaintEngine can make them rough.
class HandPaintStyle : public QProxyStyle {
    Q_OBJECT

public:
    HandPaintStyle();

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;
};

}  // namespace hpq
