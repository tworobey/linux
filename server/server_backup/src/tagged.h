#pragma once
#include <utility>
#include <functional>

namespace util {

/**
 * Маркированный тип (strong typedef)
 */
template <typename Value, typename Tag>
class Tagged {
public:
    using ValueType = Value;
    using TagType = Tag;

    Tagged() = default;

    explicit Tagged(Value v)
        : value_(std::move(v)) {}

    const Value& operator*() const {
        return value_;
    }

    Value& operator*() {
        return value_;
    }

    const Value& Get() const {
        return value_;
    }

    Value& Get() {
        return value_;
    }

    // C++17 сравнение (вместо operator<=>)
    friend bool operator==(const Tagged& lhs, const Tagged& rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend bool operator!=(const Tagged& lhs, const Tagged& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const Tagged& lhs, const Tagged& rhs) {
        return lhs.value_ < rhs.value_;
    }

private:
    Value value_{};
};

// Hash для unordered_map / unordered_set
template <typename TaggedValue>
struct TaggedHasher {
    size_t operator()(const TaggedValue& value) const {
        return std::hash<typename TaggedValue::ValueType>{}(*value);
    }
};

} // namespace util
