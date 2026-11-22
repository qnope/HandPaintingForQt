#pragma once

#include <hpq/FillStyle.h>
#include <hpq/FwdDecls.h>
#include <hpq/RoughOptions.h>

#include <QFont>
#include <QPalette>

namespace hpq {

// What a hand-painted look draws with: colors, font, roughness and fill.
class Theme {
public:
    Theme() = default;
    Theme(const Theme &) = default;
    Theme &operator=(const Theme &) = default;
    virtual ~Theme() = default;

    virtual QPalette palette() const = 0;
    virtual QFont font() const = 0;
    virtual RoughOptions rough_options() const = 0;
    virtual FillStyle fill_style() const = 0;
};

}  // namespace hpq
