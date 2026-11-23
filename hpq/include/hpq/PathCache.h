#pragma once

#include <QPainterPath>

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace hpq {

// Keeps the rough paths made for the shapes of the last repaints: a shape drawn again at the
// same place gets them back instead of having them made again.
// It forgets the shapes that were not drawn for a while, so that it stays small.
class PathCache {
public:
    // What the kept paths draw of a shape.
    enum class Kind { Outline, FrameOutline, FillArea, Hachure };

    // The paths of `kind` for `shape` and `parameter`: the kept ones, or else the ones that
    // `make` returns, which are then kept.
    template <typename Make>
    std::vector<QPainterPath> paths(Kind kind, const QPainterPath &shape, double parameter, Make make) {
        const Key key{kind, parameter, shape};
        if (const std::vector<QPainterPath> *kept = find(key)) {
            return *kept;
        }
        std::vector<QPainterPath> made = make();
        keep(key, made);
        return made;
    }

    // The number of path elements kept, which tells the memory used.
    std::size_t element_count() const;

private:
    struct Key {
        Kind kind;
        double parameter;
        QPainterPath shape;
    };
    struct KeyHash {
        std::size_t operator()(const Key &key) const;
    };
    struct KeyEqual {
        bool operator()(const Key &a, const Key &b) const;
    };
    struct Entry {
        std::vector<QPainterPath> paths;
        std::size_t element_count = 0;
    };
    using Entries = std::unordered_map<Key, Entry, KeyHash, KeyEqual>;

    const std::vector<QPainterPath> *find(const Key &key);
    void keep(const Key &key, const std::vector<QPainterPath> &paths);

    // Two generations: when the recent one is full it becomes the older one, and what was
    // only in the older one is forgotten. A shape found in the older one is moved to the recent.
    Entries m_recent;
    Entries m_older;
    std::size_t m_recent_element_count = 0;
    std::size_t m_older_element_count = 0;
};

}  // namespace hpq
