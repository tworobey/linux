#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

namespace {

class TestProvider : public collision_detector::ItemGathererProvider {
public:
    TestProvider(std::vector<collision_detector::Item> items,
                 std::vector<collision_detector::Gatherer> gatherers)
        : items_(std::move(items)), gatherers_(std::move(gatherers)) {}

    size_t ItemsCount() const override { return items_.size(); }
    collision_detector::Item GetItem(size_t idx) const override { return items_[idx]; }
    size_t GatherersCount() const override { return gatherers_.size(); }
    collision_detector::Gatherer GetGatherer(size_t idx) const override { return gatherers_[idx]; }

private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

} // namespace

TEST_CASE("FindGatherEvents detects collision", "[collision]") {
    TestProvider provider(
        { { {5.0, 0.0}, 0.0 } },
        { { {0.0, 0.0}, {10.0, 0.0}, 0.6 } }
    );
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].gatherer_id == 0);
}

TEST_CASE("FindGatherEvents no collision when item is far", "[collision]") {
    TestProvider provider(
        { { {5.0, 10.0}, 0.0 } },
        { { {0.0, 0.0}, {10.0, 0.0}, 0.6 } }
    );
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("FindGatherEvents no collision when gatherer doesn't move", "[collision]") {
    TestProvider provider(
        { { {0.0, 0.0}, 0.0 } },
        { { {0.0, 0.0}, {0.0, 0.0}, 0.6 } }
    );
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("FindGatherEvents events in chronological order", "[collision]") {
    TestProvider provider(
        { { {8.0, 0.0}, 0.0 }, { {3.0, 0.0}, 0.0 } },
        { { {0.0, 0.0}, {10.0, 0.0}, 0.6 } }
    );
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 2);
    CHECK(events[0].item_id == 1);
    CHECK(events[1].item_id == 0);
    CHECK(events[0].time < events[1].time);
}

TEST_CASE("FindGatherEvents no collision when item behind start", "[collision]") {
    TestProvider provider(
        { { {-1.0, 0.0}, 0.0 } },
        { { {0.0, 0.0}, {10.0, 0.0}, 0.6 } }
    );
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}
