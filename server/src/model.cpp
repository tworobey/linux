#include "model.h"
#include <stdexcept>
#include <random>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }
    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

static std::string GenerateToken() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist;
    uint64_t a = dist(gen);
    uint64_t b = dist(gen);
    char buf[33];
    snprintf(buf, sizeof(buf), "%016llx%016llx",
             (unsigned long long)a, (unsigned long long)b);
    return std::string(buf);
}

static DogPosition StartPositionOnMap(const Map& map) {
    const auto& roads = map.GetRoads();
    if (roads.empty()) return {0.0, 0.0};
    return { static_cast<double>(roads[0].GetStart().x),
             static_cast<double>(roads[0].GetStart().y) };
}

static DogPosition RandomPositionOnMap(const Map& map) {
    const auto& roads = map.GetRoads();
    if (roads.empty()) return {0.0, 0.0};

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
    const Road& road = roads[road_dist(gen)];

    double x0 = road.GetStart().x, y0 = road.GetStart().y;
    double x1 = road.GetEnd().x,   y1 = road.GetEnd().y;

    std::uniform_real_distribution<double> pos_dist(0.0, 1.0);
    double t = pos_dist(gen);
    return { x0 + t * (x1 - x0), y0 + t * (y1 - y0) };
}

std::pair<std::string, int> Game::JoinGame(const std::string& playerName,
                                            const std::string& mapId) {
    const Map* map = FindMap(Map::Id(mapId));

    std::string token = GenerateToken();
    int id = next_player_id_++;

    DogPosition pos = randomize_spawn_ ? RandomPositionOnMap(*map) : StartPositionOnMap(*map);
    auto dog = std::make_shared<Dog>(Dog::Id(id), playerName, pos);

    const size_t index = players_.size();
    players_.emplace_back(id, playerName, token, mapId);
    players_.back().SetDog(std::move(dog));
    token_to_player_[token] = index;

    return {token, id};
}

} // namespace model
