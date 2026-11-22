#pragma once

#include <hpq/Theme.h>

namespace hpq {

// Graphite pencil on paper.
class SketchTheme : public Theme {
public:
    // Registers the embedded "Architects Daughter" font in QFontDatabase.
    SketchTheme();

    QPalette palette() const override;
    QFont font() const override;
    RoughOptions rough_options() const override;
    FillStyle fill_style() const override;
};

}  // namespace hpq
