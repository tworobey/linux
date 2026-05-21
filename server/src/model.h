#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <random>
#include <optional>

#include "tagged.h"
#include "loot_generator.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);
    
    void SetDogSpeed(double speed) noexcept { dog_speed_ = speed; }
    double GetDogSpeed() const noexcept { return dog_speed_; }

    void SetBagCapacity(int capacity) noexcept { bag_capacity_ = capacity; }
    int GetBagCapacity() const noexcept { return bag_capacity_; }

    void SetLootTypesCount(size_t count) noexcept { loot_types_count_ = count; }
    size_t GetLootTypesCount() const noexcept { return loot_types_count_; }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dog_speed_ = 0.0;
    int bag_capacity_ = 3;
    size_t loot_types_count_ = 0;
};

struct DogPosition {
    double x = 0.0;
    double y = 0.0;
};

struct DogSpeed {
    double vx = 0.0;
    double vy = 0.0;
};

enum class DogDirection { NORTH, SOUTH, WEST, EAST };

class Dog {
public:
    using Id = util::Tagged<int, Dog>;

    Dog(Id id, std::string name, DogPosition pos) noexcept
        : id_(id), name_(std::move(name)), pos_(pos) {}

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    DogPosition GetPosition() const noexcept { return pos_; }
    DogSpeed GetSpeed() const noexcept { return speed_; }
    DogDirection GetDirection() const noexcept { return dir_; }

    void SetSpeed(DogSpeed speed) noexcept { speed_ = speed; }
    void SetDirection(DogDirection dir) noexcept { dir_ = dir; }
    void SetPosition(DogPosition pos) noexcept { pos_ = pos; }

    // Рюкзак
    struct BagItem { int id; int type; };
    const std::vector<BagItem>& GetBag() const noexcept { return bag_; }
    bool AddToBag(BagItem item, int capacity) {
        if (static_cast<int>(bag_.size()) >= capacity) return false;
        bag_.push_back(item);
        return true;
    }
    std::vector<BagItem> EmptyBag() {
        auto items = std::move(bag_);
        bag_.clear();
        return items;
    }

    // Очки
    int GetScore() const noexcept { return score_; }
    void AddScore(int points) noexcept { score_ += points; }

private:
    Id id_;
    std::string name_;
    DogPosition pos_;
    DogSpeed speed_{0.0, 0.0};
    DogDirection dir_ = DogDirection::NORTH;
    std::vector<BagItem> bag_;
    int score_ = 0;
};

class Player {
public:
    Player(int id, std::string name, std::string token, std::string mapId)
        : id_(id), name_(std::move(name))
        , token_(std::move(token)), mapId_(std::move(mapId)) {}

    int GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const std::string& GetToken() const noexcept { return token_; }
    const std::string& GetMapId() const noexcept { return mapId_; }

    void SetDog(std::shared_ptr<Dog> dog) { dog_ = std::move(dog); }
    const Dog* GetDog() const noexcept { return dog_.get(); }
    Dog* GetDog() noexcept { return dog_.get(); }

private:
    int id_;
    std::string name_;
    std::string token_;
    std::string mapId_;
    std::shared_ptr<Dog> dog_;
};

struct LostObject {
    int id = 0;
    int type = 0;
    int value = 0;
    DogPosition pos;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept { return maps_; }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end())
            return &maps_.at(it->second);
        return nullptr;
    }

    void SetDefaultDogSpeed(double speed) noexcept { default_dog_speed_ = speed; }
    double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }
    void SetRandomizeSpawn(bool val) noexcept { randomize_spawn_ = val; }
    bool GetRandomizeSpawn() const noexcept { return randomize_spawn_; }
    void SetDefaultBagCapacity(int cap) noexcept { default_bag_capacity_ = cap; }
    int GetDefaultBagCapacity() const noexcept { return default_bag_capacity_; }

    void SetLootValues(const std::string& map_id, std::vector<int> values) {
        loot_values_[map_id] = std::move(values);
    }

    void SetLootGenerator(loot_gen::LootGenerator gen) {
        loot_generator_ = std::move(gen);
    }

    const std::vector<LostObject>& GetLostObjects(const std::string& map_id) const {
        static const std::vector<LostObject> empty;
        auto it = lost_objects_.find(map_id);
        if (it == lost_objects_.end()) return empty;
        return it->second;
    }

    std::pair<std::string, int> JoinGame(const std::string& playerName,
                                          const std::string& mapId);

    const Player* FindPlayerByToken(const std::string& token) const noexcept {
        auto it = token_to_player_.find(token);
        if (it == token_to_player_.end()) return nullptr;
        return &players_.at(it->second);
    }

    Player* FindPlayerByToken(const std::string& token) noexcept {
        auto it = token_to_player_.find(token);
        if (it == token_to_player_.end()) return nullptr;
        return &players_.at(it->second);
    }

    bool MovePlayer(const std::string& token, const std::string& move) {
        Player* player = FindPlayerByToken(token);
        if (!player) return false;
        Dog* dog = const_cast<Dog*>(player->GetDog());
        if (!dog) return false;

        const Map* map = FindMap(Map::Id(player->GetMapId()));
        double s = map ? map->GetDogSpeed() : default_dog_speed_;

        if (move == "L") {
            dog->SetSpeed({-s, 0.0});
            dog->SetDirection(DogDirection::WEST);
        } else if (move == "R") {
            dog->SetSpeed({s, 0.0});
            dog->SetDirection(DogDirection::EAST);
        } else if (move == "U") {
            dog->SetSpeed({0.0, -s});
            dog->SetDirection(DogDirection::NORTH);
        } else if (move == "D") {
            dog->SetSpeed({0.0, s});
            dog->SetDirection(DogDirection::SOUTH);
        } else if (move == "") {
            dog->SetSpeed({0.0, 0.0});
        } else {
            return false; // invalid move
        }
        return true;
    }

    const std::vector<Player>& GetPlayers() const noexcept { return players_; }
    int GetNextPlayerId() const noexcept { return next_player_id_; }
    int GetNextLootId() const noexcept { return next_loot_id_; }
    void SetNextPlayerId(int id) noexcept { next_player_id_ = id; }
    void SetNextLootId(int id) noexcept { next_loot_id_ = id; }

    const std::unordered_map<std::string, std::vector<LostObject>>& GetAllLostObjects() const noexcept {
        return lost_objects_;
    }

    void RestorePlayer(int id, const std::string& name, const std::string& token,
                       const std::string& map_id, Dog dog) {
        const size_t index = players_.size();
        players_.emplace_back(id, name, token, map_id);
        auto dog_ptr = std::make_shared<Dog>(std::move(dog));
        players_.back().SetDog(dog_ptr);
        token_to_player_[token] = index;
    }

    void RestoreLostObjects(const std::string& map_id, const std::vector<LostObject>& objects) {
        lost_objects_[map_id] = objects;
    }

    std::vector<const Player*> GetPlayersOnMap(const std::string& mapId) const {
        std::vector<const Player*> result;
        for (const auto& p : players_)
            if (p.GetMapId() == mapId)
                result.push_back(&p);
        return result;
    }

    // Обновляет состояние игры на dt миллисекунд
    void Tick(double dt_ms) {
        constexpr double MILLISECONDS_PER_SECOND = 1000.0;
        double dt = dt_ms / MILLISECONDS_PER_SECOND;

        // Сохраняем позиции до движения
        std::unordered_map<int, DogPosition> prev_positions;
        for (const auto& player : players_) {
            const Dog* dog = player.GetDog();
            if (dog) prev_positions[*dog->GetId()] = dog->GetPosition();
        }

        // Двигаем собак
        for (auto& player : players_) {
            Dog* dog = player.GetDog();
            if (!dog) continue;
            const Map* map = FindMap(Map::Id(player.GetMapId()));
            if (!map) continue;
            MoveDog(*dog, *map, dt);
        }

        // Обрабатываем коллизии
        ProcessCollisions(prev_positions);

        // Генерируем лут на каждой карте
        if (loot_generator_) {
            auto time_delta = std::chrono::milliseconds(static_cast<int>(dt_ms));
            for (const auto& map : maps_) {
                const std::string map_id = *map.GetId();
                auto& lost = lost_objects_[map_id];
                unsigned loot_count = static_cast<unsigned>(lost.size());
                unsigned looter_count = static_cast<unsigned>(
                    GetPlayersOnMap(map_id).size());
                unsigned new_loot = loot_generator_->Generate(
                    time_delta, loot_count, looter_count);

                auto values_it = loot_values_.find(map_id);
                for (unsigned i = 0; i < new_loot; ++i) {
                    LostObject obj;
                    obj.id = next_loot_id_++;
                    obj.type = RandomLootType(map);
                    obj.pos = RandomPositionOnMap(map);
                    if (values_it != loot_values_.end() &&
                        obj.type < static_cast<int>(values_it->second.size()))
                        obj.value = values_it->second[obj.type];
                    lost.push_back(obj);
                }
            }
        }
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::vector<Player> players_;
    std::unordered_map<std::string, size_t> token_to_player_;
    int next_player_id_ = 0;
    int next_loot_id_ = 0;
    double default_dog_speed_ = 1.0;
    int default_bag_capacity_ = 3;
    bool randomize_spawn_ = false;
    std::optional<loot_gen::LootGenerator> loot_generator_;
    std::unordered_map<std::string, std::vector<LostObject>> lost_objects_;
    std::unordered_map<std::string, std::vector<int>> loot_values_;

    void ProcessCollisions(const std::unordered_map<int, DogPosition>& prev_positions) {
        for (auto& player : players_) {
            Dog* dog = player.GetDog();
            if (!dog) continue;
            const Map* map = FindMap(Map::Id(player.GetMapId()));
            if (!map) continue;

            DogPosition prev_pos = prev_positions.count(*dog->GetId())
                ? prev_positions.at(*dog->GetId()) : dog->GetPosition();
            DogPosition cur_pos = dog->GetPosition();

            // Пропускаем если не двигался
            if (prev_pos.x == cur_pos.x && prev_pos.y == cur_pos.y) {
                // Проверяем базы даже стоя
            }

            int bag_capacity = map->GetBagCapacity() > 0
                ? map->GetBagCapacity() : default_bag_capacity_;

            const std::string& map_id = player.GetMapId();
            auto& lost = lost_objects_[map_id];

            // Проверяем подбор предметов
            std::vector<int> indices_to_remove;
            for (int idx = 0; idx < static_cast<int>(lost.size()); ++idx) {
                const auto& obj = lost.at(idx);
                // Расстояние от точки до отрезка перемещения
                double move_dx = cur_pos.x - prev_pos.x;
                double move_dy = cur_pos.y - prev_pos.y;
                double move_length_sq = move_dx*move_dx + move_dy*move_dy;

                double sq_dist_to_item;
                if (move_length_sq < 1e-20) {
                    double item_dx = obj.pos.x - prev_pos.x;
                    double item_dy = obj.pos.y - prev_pos.y;
                    sq_dist_to_item = item_dx*item_dx + item_dy*item_dy;
                } else {
                    double item_dx = obj.pos.x - prev_pos.x;
                    double item_dy = obj.pos.y - prev_pos.y;
                    double proj = (item_dx*move_dx + item_dy*move_dy) / move_length_sq;
                    if (proj < 0.0) proj = 0.0;
                    if (proj > 1.0) proj = 1.0;
                    double perp_x = prev_pos.x + proj*move_dx - obj.pos.x;
                    double perp_y = prev_pos.y + proj*move_dy - obj.pos.y;
                    sq_dist_to_item = perp_x*perp_x + perp_y*perp_y;
                }

                const double collect_radius = 0.3; // 0.6/2
                if (sq_dist_to_item <= collect_radius * collect_radius) {
                    Dog::BagItem item{obj.id, obj.type};
                    if (dog->AddToBag(item, bag_capacity)) {
                        indices_to_remove.push_back(idx);
                    }
                }
            }

            // Удаляем подобранные предметы (с конца чтобы не сбить индексы)
            for (int i = static_cast<int>(indices_to_remove.size()) - 1; i >= 0; --i) {
                lost.erase(lost.begin() + indices_to_remove.at(i));
            }

            // Проверяем возврат на базу
            const double base_radius = 0.55; // 0.5/2 + 0.6/2
            for (const auto& office : map->GetOffices()) {
                double office_x = office.GetPosition().x;
                double office_y = office.GetPosition().y;

                double move_dx = cur_pos.x - prev_pos.x;
                double move_dy = cur_pos.y - prev_pos.y;
                double move_length_sq = move_dx*move_dx + move_dy*move_dy;

                double sq_dist_to_base;
                if (move_length_sq < 1e-20) {
                    double base_dx = office_x - prev_pos.x;
                    double base_dy = office_y - prev_pos.y;
                    sq_dist_to_base = base_dx*base_dx + base_dy*base_dy;
                } else {
                    double base_dx = office_x - prev_pos.x;
                    double base_dy = office_y - prev_pos.y;
                    double proj = (base_dx*move_dx + base_dy*move_dy) / move_length_sq;
                    if (proj < 0.0) proj = 0.0;
                    if (proj > 1.0) proj = 1.0;
                    double perp_x = prev_pos.x + proj*move_dx - office_x;
                    double perp_y = prev_pos.y + proj*move_dy - office_y;
                    sq_dist_to_base = perp_x*perp_x + perp_y*perp_y;
                }

                if (sq_dist_to_base <= base_radius * base_radius) {
                    // Сдаём предметы
                    auto items = dog->EmptyBag();
                    const std::string mid = player.GetMapId();
                    auto val_it = loot_values_.find(mid);
                    for (const auto& item : items) {
                        int points = 0;
                        if (val_it != loot_values_.end() &&
                            item.type < static_cast<int>(val_it->second.size()))
                            points = val_it->second[item.type];
                        dog->AddScore(points);
                    }
                    break;
                }
            }
        }
    }

    static int RandomLootType(const Map& map) {
        if (map.GetLootTypesCount() == 0) return 0;
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(
            0, static_cast<int>(map.GetLootTypesCount()) - 1);
        return dist(gen);
    }

    static DogPosition RandomPositionOnMap(const Map& map) {
        const auto& roads = map.GetRoads();
        if (roads.empty()) return {0.0, 0.0};
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
        const Road& road = roads[road_dist(gen)];
        double x0 = road.GetStart().x, y0 = road.GetStart().y;
        double x1 = road.GetEnd().x,   y1 = road.GetEnd().y;
        std::uniform_real_distribution<double> pos_dist(0.0, 1.0);
        double t = pos_dist(gen);
        return { x0 + t * (x1 - x0), y0 + t * (y1 - y0) };
    }

    // Перемещает собаку с учётом границ дорог
    static void MoveDog(Dog& dog, const Map& map, double dt) {
        DogSpeed spd = dog.GetSpeed();
        if (spd.vx == 0.0 && spd.vy == 0.0) return;

        DogPosition pos = dog.GetPosition();
        double new_x = pos.x + spd.vx * dt;
        double new_y = pos.y + spd.vy * dt;

        // Ищем дорогу, на которой стоит пёс, и ограничиваем движение
        const double HALF_WIDTH = 0.4;
        const auto& roads = map.GetRoads();

        // Находим все дороги где сейчас стоит пёс
        double best_x = pos.x, best_y = pos.y;
        bool moved = false;

        for (const auto& road : roads) {
            double rx0 = std::min(road.GetStart().x, road.GetEnd().x);
            double rx1 = std::max(road.GetStart().x, road.GetEnd().x);
            double ry0 = std::min(road.GetStart().y, road.GetEnd().y);
            double ry1 = std::max(road.GetStart().y, road.GetEnd().y);

            // Расширяем дорогу на HALF_WIDTH
            double left   = rx0 - HALF_WIDTH;
            double right  = rx1 + HALF_WIDTH;
            double top    = ry0 - HALF_WIDTH;
            double bottom = ry1 + HALF_WIDTH;

            // Пёс должен быть на этой дороге
            if (pos.x < left - 1e-9 || pos.x > right + 1e-9) continue;
            if (pos.y < top  - 1e-9 || pos.y > bottom + 1e-9) continue;

            // Ограничиваем новую позицию границами дороги
            double clamped_x = std::clamp(new_x, left, right);
            double clamped_y = std::clamp(new_y, top, bottom);

            // Выбираем позицию максимально близкую к желаемой
            double dist = std::abs(clamped_x - pos.x) + std::abs(clamped_y - pos.y);
            double best_dist = moved ? (std::abs(best_x - pos.x) + std::abs(best_y - pos.y)) : -1.0;

            if (!moved || dist > best_dist) {
                best_x = clamped_x;
                best_y = clamped_y;
                moved = true;
            }
        }

        if (!moved) return;

        // Если упёрся в границу — останавливаем
        if (std::abs(best_x - new_x) > 1e-9 || std::abs(best_y - new_y) > 1e-9) {
            dog.SetSpeed({0.0, 0.0});
        }
        dog.SetPosition({best_x, best_y});
    }
};
}  // namespace model
