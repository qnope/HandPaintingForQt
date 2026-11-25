#include <hpq/PaintCache.h>

#include <algorithm>
#include <cstring>

namespace hpq {

namespace {

constexpr std::size_t BytesPerMegabyte = 1024 * 1024;
// A generation of paths holds at most this many bytes: the paths of a large window take
// one or two megabytes.
constexpr std::size_t MaxPathBytesPerGeneration = 4 * BytesPerMegabyte;
// A generation of pieces holds at most this many bytes: those of a window of 1400 x 1000 at a
// device pixel ratio of 2 take about ten megabytes.
constexpr std::size_t MaxPieceBytesPerGeneration = 16 * BytesPerMegabyte;
// A generation of washes holds at most this many bytes. A wash takes four bytes for each
// pixel of the device that it covers.
constexpr std::size_t MaxWashBytesPerGeneration = 16 * BytesPerMegabyte;
// A wash that would take more than this part of a generation is not kept.
constexpr std::size_t LargestWashPart = 4;

// What a path takes: its elements, and about this much more for the path itself.
constexpr std::size_t BytesPerPath = 96;
// The hash of a path mixes at most about this many of its elements: two different paths of the
// same size differ almost always there, and the keys are then compared whole.
constexpr int HashedElements = 32;

std::uint64_t bits_of(double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void mix(std::uint64_t &hash, std::uint64_t value) {
    hash = (hash ^ value) * 0x9e3779b97f4a7c15ULL;
    hash ^= hash >> 29;
}

std::size_t byte_count_of_path(const QPainterPath &path) {
    return BytesPerPath + static_cast<std::size_t>(path.elementCount()) * sizeof(QPainterPath::Element);
}

}  // namespace

PaintCache::PaintCache()
    : m_paths(MaxPathBytesPerGeneration), m_pieces(MaxPieceBytesPerGeneration), m_washes(MaxWashBytesPerGeneration) {}

std::size_t PaintCache::largest_kept_wash() { return MaxWashBytesPerGeneration / LargestWashPart; }

std::size_t PaintCache::byte_count() const {
    return m_paths.byte_count() + m_pieces.byte_count() + m_washes.byte_count();
}

void PaintCache::clear() {
    m_paths.clear();
    m_pieces.clear();
    m_washes.clear();
}

std::size_t PaintCache::KeyHash::operator()(const Key &key) const {
    std::uint64_t hash = 0x243f6a8885a308d3ULL;
    mix(hash, key.kind);
    mix(hash, key.variant);
    mix(hash, bits_of(key.parameter));
    mix(hash, static_cast<std::uint64_t>(key.shape.fillRule()));
    const int count = key.shape.elementCount();
    mix(hash, static_cast<std::uint64_t>(count));
    const int step = std::max(1, count / HashedElements);
    for (int index = 0; index < count; index += step) {
        const QPainterPath::Element element = key.shape.elementAt(index);
        mix(hash, static_cast<std::uint64_t>(element.type));
        mix(hash, bits_of(element.x));
        mix(hash, bits_of(element.y));
    }
    return static_cast<std::size_t>(hash);
}

// QPainterPath::operator== compares the coordinates loosely: here they are compared bit for bit.
bool PaintCache::KeyEqual::operator()(const Key &first, const Key &second) const {
    if (first.kind != second.kind || first.variant != second.variant ||
        bits_of(first.parameter) != bits_of(second.parameter) ||
        first.shape.fillRule() != second.shape.fillRule() ||
        first.shape.elementCount() != second.shape.elementCount()) {
        return false;
    }
    for (int index = 0; index < first.shape.elementCount(); ++index) {
        const QPainterPath::Element first_element = first.shape.elementAt(index);
        const QPainterPath::Element second_element = second.shape.elementAt(index);
        if (first_element.type != second_element.type || bits_of(first_element.x) != bits_of(second_element.x) ||
            bits_of(first_element.y) != bits_of(second_element.y)) {
            return false;
        }
    }
    return true;
}

std::uint64_t PaintCache::phase_bits(const QPointF &phase) {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    const float x_phase = static_cast<float>(phase.x());
    const float y_phase = static_cast<float>(phase.y());
    std::memcpy(&x, &x_phase, sizeof(x));
    std::memcpy(&y, &y_phase, sizeof(y));
    return (static_cast<std::uint64_t>(x) << 32) | y;
}

std::size_t PaintCache::byte_count_of(const Key &key) { return sizeof(Key) + byte_count_of_path(key.shape); }

std::size_t PaintCache::byte_count_of(const std::vector<QPainterPath> &paths) {
    std::size_t byte_count = 0;
    for (const QPainterPath &path : paths) {
        byte_count += byte_count_of_path(path);
    }
    return byte_count;
}

}  // namespace hpq
