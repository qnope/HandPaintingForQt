#include <hpq/GeometryEngine.h>

namespace hpq {

std::vector<QPainterPath> GeometryEngine::frame_outline(const QPainterPath &frame, const RoughOptions &options,
                                                        std::uint32_t seed) const {
    std::vector<QPainterPath> strokes = outline(frame, options, seed);
    for (QPainterPath &stroke : strokes) {
        stroke.setFillRule(frame.fillRule());
        stroke = stroke.united(frame);
    }
    return strokes;
}

}  // namespace hpq
