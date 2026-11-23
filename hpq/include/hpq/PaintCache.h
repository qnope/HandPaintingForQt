#pragma once

#include <QImage>
#include <QPainterPath>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hpq {

// Keeps what a hand paint engine made to draw its last shapes: their rough paths, and the
// images of their washes. A shape that is drawn again gets them back instead of having them
// made again, which is most of the work of a repaint.
// What is kept depends only on the shape and on how it is drawn, not on where: a cache serves
// one theme and one geometry engine, and the shapes of a view that scrolls find their paths.
// It forgets what was not drawn for a while, so that it stays small.
class PaintCache {
public:
    // What the kept paths draw of a shape.
    enum class PathKind { Outline, FrameOutline, FillArea, Hachure };

    PaintCache();

    // The paths of `kind` for `shape`, drawn by the tool numbered `tool` with a line of
    // `width` pixels: the kept ones, or else the ones that `make` returns, which are then kept.
    template <typename Make>
    std::vector<QPainterPath> paths(PathKind kind, const QPainterPath &shape, int tool, double width, Make make) {
        const Key key{static_cast<std::uint32_t>(kind), static_cast<std::uint64_t>(tool), width, shape};
        if (const std::vector<QPainterPath> *kept = m_paths.find(key)) {
            return *kept;
        }
        std::vector<QPainterPath> made = make();
        m_paths.keep(key, made, byte_count_of(made));
        return made;
    }

    // The image of the wash numbered `kind` that `pigment` leaves on `area`, for a device of
    // `device_pixel_ratio`: the kept one, or else the one that `make` returns, which is then kept.
    template <typename Make>
    QImage wash(const QPainterPath &area, std::uint64_t pigment, int kind, double device_pixel_ratio, Make make) {
        const Key key{static_cast<std::uint32_t>(kind), pigment, device_pixel_ratio, area};
        if (const QImage *kept = m_washes.find(key)) {
            return *kept;
        }
        QImage made = make();
        m_washes.keep(key, made, static_cast<std::size_t>(made.sizeInBytes()));
        return made;
    }

    // An image larger than this many bytes is not worth keeping: it would push out the others.
    static std::size_t largest_kept_wash();

    // About how many bytes the kept paths and images take.
    std::size_t byte_count() const;

    // Forgets everything.
    void clear();

private:
    // What a kept value was made from. Two keys are the same when they are the same bit for
    // bit: a shape that moved by a hair is another shape.
    struct Key {
        std::uint32_t kind;
        std::uint64_t variant;
        double parameter;
        QPainterPath shape;
    };
    struct KeyHash {
        std::size_t operator()(const Key &key) const;
    };
    struct KeyEqual {
        bool operator()(const Key &first, const Key &second) const;
    };
    static std::size_t byte_count_of(const Key &key);
    static std::size_t byte_count_of(const std::vector<QPainterPath> &paths);

    // Two generations of kept values: when the recent one is full it becomes the older one,
    // and what was only in the older one is forgotten. A value found in the older one moves
    // to the recent one: what is drawn at every repaint is never forgotten.
    template <typename Value>
    class Generations {
    public:
        explicit Generations(std::size_t max_byte_count) : m_max_byte_count(max_byte_count) {}

        const Value *find(const Key &key) {
            const auto recent = m_recent.find(key);
            if (recent != m_recent.end()) {
                return &recent->second.value;
            }
            const auto older = m_older.find(key);
            if (older == m_older.end()) {
                return nullptr;
            }
            Entry entry = std::move(older->second);
            m_older_byte_count -= entry.byte_count;
            m_older.erase(older);
            make_room_for(entry.byte_count);
            m_recent_byte_count += entry.byte_count;
            return &m_recent.emplace(key, std::move(entry)).first->second.value;
        }

        void keep(const Key &key, const Value &value, std::size_t value_byte_count) {
            const std::size_t byte_count = byte_count_of(key) + value_byte_count;
            make_room_for(byte_count);
            m_recent_byte_count += byte_count;
            m_recent.insert_or_assign(key, Entry{value, byte_count});
        }

        std::size_t byte_count() const { return m_recent_byte_count + m_older_byte_count; }

        void clear() {
            m_recent.clear();
            m_older.clear();
            m_recent_byte_count = 0;
            m_older_byte_count = 0;
        }

    private:
        struct Entry {
            Value value;
            std::size_t byte_count = 0;
        };
        using Entries = std::unordered_map<Key, Entry, KeyHash, KeyEqual>;

        // Ends the recent generation when `byte_count` more bytes would make it too large.
        void make_room_for(std::size_t byte_count) {
            if (m_recent.empty() || m_recent_byte_count + byte_count <= m_max_byte_count) {
                return;
            }
            m_older = std::move(m_recent);
            m_older_byte_count = m_recent_byte_count;
            m_recent = Entries();
            m_recent_byte_count = 0;
        }

        std::size_t m_max_byte_count;
        Entries m_recent;
        Entries m_older;
        std::size_t m_recent_byte_count = 0;
        std::size_t m_older_byte_count = 0;
    };

    Generations<std::vector<QPainterPath>> m_paths;
    Generations<QImage> m_washes;
};

}  // namespace hpq
