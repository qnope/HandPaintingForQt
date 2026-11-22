#pragma once

#include <hpq/FwdDecls.h>

#include <QColor>

namespace hpq {

// How the inside of a closed shape is painted.
enum class FillStyle { Solid };

// A plain value describing how a drawing looks once hand-painted.
class Theme
{
public:
    Theme(QColor background, QColor ink, QColor fill, double stroke_width, double roughness, int passes,
          FillStyle fill_style);

    static Theme sketch();
    static Theme china_ink();
    static Theme chalk();

    QColor background() const { return m_background; }
    QColor ink() const { return m_ink; }
    QColor fill() const { return m_fill; }
    double stroke_width() const { return m_stroke_width; }
    double roughness() const { return m_roughness; }
    int passes() const { return m_passes; }
    FillStyle fill_style() const { return m_fill_style; }

    friend bool operator==(const Theme& lhs, const Theme& rhs);
    friend bool operator!=(const Theme& lhs, const Theme& rhs) { return !(lhs == rhs); }

private:
    QColor m_background;
    QColor m_ink;
    QColor m_fill;
    double m_stroke_width;
    double m_roughness;
    int m_passes;
    FillStyle m_fill_style;
};

} // namespace hpq
