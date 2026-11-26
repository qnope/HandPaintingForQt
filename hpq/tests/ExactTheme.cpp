#include "ExactTheme.h"

namespace hpq::test {

Theme with_rough_options(const Theme &theme, const RoughOptions &options) {
    return Theme(theme.palette(), theme.font(), options, theme.fill_style(), theme.medium(), theme.text_ink(),
                 theme.label_marker())
        .with_antialiasing_mode(theme.antialiasing_mode());
}

Theme exact(const Theme &theme) {
    RoughOptions options = theme.rough_options();
    options.roughness = 0.0;
    options.stroke_count = 1;
    return with_rough_options(theme, options);
}

}  // namespace hpq::test
