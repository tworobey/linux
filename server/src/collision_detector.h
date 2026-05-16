#pragma once

#include <vector>
#include <cassert>
#include <cmath>

namespace collision_detector {

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

struct Item {
    Point2D position;
    double width = 0.0;
};

struct Gatherer {
    Point2D start_pos;
    Point2D end_pos;
    double width = 0.0;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;
public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

} // namespace collision_detector
