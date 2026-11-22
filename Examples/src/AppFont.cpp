#include "AppFont.h"

#include <QApplication>
#include <QFont>

namespace hpq::example {
void apply_default_font()
{
    QApplication::setFont(QFont("Helvetica"));
}
} // namespace hpq::example
