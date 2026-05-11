#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

#include "tagged.h"

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

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dog_speed_ = 0.0;
};

struct DogPosition {
    double x = 0.0, y = 0.0;
};

struct DogSpeed {
    double vx = 0.0, vy = 0.0;
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

private:
    Id id_;
    std::string name_;
    DogPosition pos_;
    DogSpeed speed_{0.0, 0.0};
    DogDirection dir_ = DogDirection::NORTH;
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

    std::vector<const Player*> GetPlayersOnMap(const std::string& mapId) const {
        std::vector<const Player*> result;
        for (auto& p : players_)
            if (p.GetMapId() == mapId)
                result.push_back(&p);
        return result;
    }

    // Обновляет состояние игры на dt миллисекунд
    void Tick(double dt_ms) {
        double dt = dt_ms / 1000.0; // переводим в секунды
        for (auto& player : players_) {
            Dog* dog = player.GetDog();
            if (!dog) continue;
            const Map* map = FindMap(Map::Id(player.GetMapId()));
            if (!map) continue;
            MoveDog(*dog, *map, dt);
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
    double default_dog_speed_ = 1.0;
    bool randomize_spawn_ = false;

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
