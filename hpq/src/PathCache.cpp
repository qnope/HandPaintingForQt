#include <hpq/PathCache.h>

#include <cstdint>
#include <cstring>
#include <utility>

namespace hpq {

namespace {

// The recent generation holds at most this many path elements (24 bytes each).
constexpr std::size_t MaxElementsPerGeneration = 400000;

std::uint64_t bits_of(double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void mix(std::uint64_t &hash, std::uint64_t value) {
    hash = (hash ^ value) * 0x9e3779b97f4a7c15ULL;
    hash ^= hash >> 29;
}

std::size_t element_count_of(const QPainterPath &shape, const std::vector<QPainterPath> &paths) {
    auto count = static_cast<std::size_t>(shape.elementCount());
    for (const QPainterPath &path : paths) {
        count += static_cast<std::size_t>(path.elementCount());
    }
    return count;
}

}  // namespace

std::size_t PathCache::KeyHash::operator()(const Key &key) const {
    std::uint64_t hash = 0x243f6a8885a308d3ULL;
    mix(hash, static_cast<std::uint64_t>(key.kind));
    mix(hash, bits_of(key.parameter));
    mix(hash, static_cast<std::uint64_t>(key.shape.fillRule()));
    for (int index = 0; index < key.shape.elementCount(); ++index) {
        const QPainterPath::Element element = key.shape.elementAt(index);
        mix(hash, static_cast<std::uint64_t>(element.type));
        mix(hash, bits_of(element.x));
        mix(hash, bits_of(element.y));
    }
    return static_cast<std::size_t>(hash);
}

// The same elements, bit for bit: QPainterPath::operator== compares them loosely.
bool PathCache::KeyEqual::operator()(const Key &a, const Key &b) const {
    if (a.kind != b.kind || bits_of(a.parameter) != bits_of(b.parameter) ||
        a.shape.fillRule() != b.shape.fillRule() || a.shape.elementCount() != b.shape.elementCount()) {
        return false;
    }
    for (int index = 0; index < a.shape.elementCount(); ++index) {
        const QPainterPath::Element first = a.shape.elementAt(index);
        const QPainterPath::Element second = b.shape.elementAt(index);
        if (first.type != second.type || bits_of(first.x) != bits_of(second.x) ||
            bits_of(first.y) != bits_of(second.y)) {
            return false;
        }
    }
    return true;
}

std::size_t PathCache::element_count() const {
    return m_recent_element_count + m_older_element_count;
}

const std::vector<QPainterPath> *PathCache::find(const Key &key) {
    const auto recent = m_recent.find(key);
    if (recent != m_recent.end()) {
        return &recent->second.paths;
    }
    const auto older = m_older.find(key);
    if (older == m_older.end()) {
        return nullptr;
    }
    // Drawn again: it moves to the recent generation.
    Entry entry = std::move(older->second);
    m_older_element_count -= entry.element_count;
    m_older.erase(older);
    m_recent_element_count += entry.element_count;
    return &m_recent.emplace(key, std::move(entry)).first->second.paths;
}

void PathCache::keep(const Key &key, const std::vector<QPainterPath> &paths) {
    const std::size_t element_count = element_count_of(key.shape, paths);
    if (m_recent_element_count + element_count > MaxElementsPerGeneration) {
        m_older = std::move(m_recent);
        m_older_element_count = m_recent_element_count;
        m_recent = Entries();
        m_recent_element_count = 0;
    }
    m_recent_element_count += element_count;
    m_recent.emplace(key, Entry{paths, element_count});
}

}  // namespace hpq
