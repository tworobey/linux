#pragma once
#include <string>
#include <unordered_map>
#include <boost/json.hpp>

namespace extra_data {

// Хранит JSON lootTypes для каждой карты (вне модели)
class MapExtraData {
public:
    void SetLootTypes(const std::string& map_id, boost::json::array loot_types) {
        loot_types_[map_id] = std::move(loot_types);
    }

    const boost::json::array* GetLootTypes(const std::string& map_id) const {
        auto it = loot_types_.find(map_id);
        return it != loot_types_.end() ? &it->second : nullptr;
    }

private:
    std::unordered_map<std::string, boost::json::array> loot_types_;
};

} // namespace extra_data
