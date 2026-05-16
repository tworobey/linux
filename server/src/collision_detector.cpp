#include "collision_detector.h"
#include <algorithm>

namespace collision_detector {

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;

    for (size_t g = 0; g < provider.GatherersCount(); ++g) {
        const Gatherer gatherer = provider.GetGatherer(g);

        // Если собиратель не двигался — пропускаем
        double dx = gatherer.end_pos.x - gatherer.start_pos.x;
        double dy = gatherer.end_pos.y - gatherer.start_pos.y;
        double v_len2 = dx * dx + dy * dy;
        if (v_len2 == 0.0) continue;

        for (size_t i = 0; i < provider.ItemsCount(); ++i) {
            const Item item = provider.GetItem(i);

            double ux = item.position.x - gatherer.start_pos.x;
            double uy = item.position.y - gatherer.start_pos.y;

            double u_dot_v = ux * dx + uy * dy;
            double u_len2 = ux * ux + uy * uy;
            double proj_ratio = u_dot_v / v_len2;
            double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

            double collect_radius = gatherer.width / 2.0 + item.width / 2.0;

            if (proj_ratio >= 0.0 && proj_ratio <= 1.0 &&
                sq_distance <= collect_radius * collect_radius) {
                events.push_back({i, g, sq_distance, proj_ratio});
            }
        }
    }

    // Сортируем по времени (хронологический порядок)
    std::sort(events.begin(), events.end(),
        [](const GatheringEvent& a, const GatheringEvent& b) {
            return a.time < b.time;
        });

    return events;
}

} // namespace collision_detector
