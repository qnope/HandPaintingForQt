#include <hpq/Theme.h>

#include <utility>

namespace hpq {

Theme::Theme(QColor background, QColor ink, QColor fill, double stroke_width, double roughness, int passes,
             FillStyle fill_style)
    : m_background(std::move(background))
    , m_ink(std::move(ink))
    , m_fill(std::move(fill))
    , m_stroke_width(stroke_width)
    , m_roughness(roughness)
    , m_passes(passes)
    , m_fill_style(fill_style)
{
}

Theme Theme::sketch()
{
    return Theme(QColor("#FFFFFF"), QColor("#333333"), QColor("#D0D0D0"), 1.0, 1.5, 2, FillStyle::Solid);
}

Theme Theme::china_ink()
{
    return Theme(QColor("#F5EFE0"), QColor("#000000"), QColor("#808080"), 2.5, 1.0, 1, FillStyle::Solid);
}

Theme Theme::chalk()
{
    return Theme(QColor("#2F3B33"), QColor("#F0F0E8"), QColor("#A0A8A0"), 2.0, 2.0, 2, FillStyle::Solid);
}

bool operator==(const Theme& lhs, const Theme& rhs)
{
    return lhs.m_background == rhs.m_background && lhs.m_ink == rhs.m_ink && lhs.m_fill == rhs.m_fill
        && lhs.m_stroke_width == rhs.m_stroke_width && lhs.m_roughness == rhs.m_roughness
        && lhs.m_passes == rhs.m_passes && lhs.m_fill_style == rhs.m_fill_style;
}

} // namespace hpq
