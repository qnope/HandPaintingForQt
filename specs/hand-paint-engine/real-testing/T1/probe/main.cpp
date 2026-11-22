#include <hpq/Theme.h>

#include <cstdio>
#include <string>

namespace {

int failures = 0;

const char* style_name(hpq::FillStyle s)
{
    switch (s) {
    case hpq::FillStyle::Solid: return "Solid";
    }
    return "?";
}

void check(const char* name, const hpq::Theme& t, const char* bg, const char* ink, const char* fill, double width,
           double rough, int passes)
{
    std::string got_bg = t.background().name(QColor::HexRgb).toUpper().toStdString();
    std::string got_ink = t.ink().name(QColor::HexRgb).toUpper().toStdString();
    std::string got_fill = t.fill().name(QColor::HexRgb).toUpper().toStdString();
    std::printf("%-10s background=%s (alpha %d) ink=%s (alpha %d) fill=%s (alpha %d) stroke_width=%.2f roughness=%.2f "
                "passes=%d fill_style=%s\n",
                name, got_bg.c_str(), t.background().alpha(), got_ink.c_str(), t.ink().alpha(), got_fill.c_str(),
                t.fill().alpha(), t.stroke_width(), t.roughness(), t.passes(), style_name(t.fill_style()));
    bool ok = got_bg == bg && got_ink == ink && got_fill == fill && t.stroke_width() == width && t.roughness() == rough
              && t.passes() == passes && t.fill_style() == hpq::FillStyle::Solid && t.background().alpha() == 255
              && t.ink().alpha() == 255 && t.fill().alpha() == 255;
    std::printf("  -> %s\n", ok ? "MATCHES SPEC" : "DIFFERS FROM SPEC");
    if (!ok)
        ++failures;
}

} // namespace

int main()
{
    check("sketch", hpq::Theme::sketch(), "#FFFFFF", "#333333", "#D0D0D0", 1.0, 1.5, 2);
    check("china_ink", hpq::Theme::china_ink(), "#F5EFE0", "#000000", "#808080", 2.5, 1.0, 1);
    check("chalk", hpq::Theme::chalk(), "#2F3B33", "#F0F0E8", "#A0A8A0", 2.0, 2.0, 2);

    // A plain value: copy, compare, assign.
    hpq::Theme copy = hpq::Theme::sketch();
    bool value_ok = copy == hpq::Theme::sketch() && copy != hpq::Theme::chalk();
    copy = hpq::Theme::chalk();
    value_ok = value_ok && copy == hpq::Theme::chalk() && hpq::Theme::sketch() != hpq::Theme::china_ink();
    std::printf("value semantics (copy/assign/==/!=): %s\n", value_ok ? "OK" : "WRONG");
    if (!value_ok)
        ++failures;

    std::printf("%s\n", failures == 0 ? "ALL PRESETS MATCH SPEC" : "SOME PRESETS DIFFER");
    return failures == 0 ? 0 : 1;
}
