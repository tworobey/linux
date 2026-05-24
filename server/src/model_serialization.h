#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "model.h"

namespace model {

template <typename Archive>
void serialize(Archive& ar, DogPosition& pos, [[maybe_unused]] const unsigned version) {
    ar& pos.x;
    ar& pos.y;
}

template <typename Archive>
void serialize(Archive& ar, DogSpeed& speed, [[maybe_unused]] const unsigned version) {
    ar& speed.vx;
    ar& speed.vy;
}

template <typename Archive>
void serialize(Archive& ar, LostObject& obj, [[maybe_unused]] const unsigned version) {
    ar& obj.id;
    ar& obj.type;
    ar& obj.value;
    ar& obj.pos;
}

} // namespace model

namespace serialization {

class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(*dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , speed_(dog.GetSpeed())
        , direction_(static_cast<int>(dog.GetDirection()))
        , score_(dog.GetScore()) {
        for (const auto& item : dog.GetBag()) {
            bag_.push_back({item.id, item.type});
        }
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog(model::Dog::Id{id_}, name_, pos_);
        dog.SetSpeed(speed_);
        dog.SetDirection(static_cast<model::DogDirection>(direction_));
        dog.AddScore(score_);
        for (const auto& item : bag_) {
            dog.AddToBag({item.first, item.second}, 1000);
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_;
    }

private:
    int id_ = 0;
    std::string name_;
    model::DogPosition pos_;
    model::DogSpeed speed_;
    int direction_ = 0;
    int score_ = 0;
    std::vector<std::pair<int, int>> bag_;
};

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const model::Player& player, const model::Dog& dog)
        : id_(player.GetId())
        , name_(player.GetName())
        , token_(player.GetToken())
        , map_id_(player.GetMapId())
        , dog_(dog) {}

    int GetId() const { return id_; }
    const std::string& GetToken() const { return token_; }
    const std::string& GetMapId() const { return map_id_; }
    const DogRepr& GetDogRepr() const { return dog_; }
    const std::string& GetName() const { return name_; }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& token_;
        ar& map_id_;
        ar& dog_;
    }

private:
    int id_ = 0;
    std::string name_;
    std::string token_;
    std::string map_id_;
    DogRepr dog_;
};

class GameStateRepr {
public:
    GameStateRepr() = default;

    explicit GameStateRepr(const model::Game& game) {
        next_player_id_ = game.GetNextPlayerId();
        next_loot_id_ = game.GetNextLootId();

        for (const auto& player : game.GetPlayers()) {
            const model::Dog* dog = player.GetDog();
            if (dog) {
                players_.emplace_back(player, *dog);
            }
        }

        for (const auto& [map_id, lost] : game.GetAllLostObjects()) {
            lost_objects_[map_id] = lost;
        }
    }

    void Restore(model::Game& game) const {
        game.SetNextPlayerId(next_player_id_);
        game.SetNextLootId(next_loot_id_);

        for (const auto& p_repr : players_) {
            model::Dog dog = p_repr.GetDogRepr().Restore();
            game.RestorePlayer(p_repr.GetId(), p_repr.GetName(),
                               p_repr.GetToken(), p_repr.GetMapId(),
                               std::move(dog));
        }

        for (const auto& [map_id, lost] : lost_objects_) {
            game.RestoreLostObjects(map_id, lost);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& next_player_id_;
        ar& next_loot_id_;
        ar& players_;
        ar& lost_objects_;
    }

private:
    int next_player_id_ = 0;
    int next_loot_id_ = 0;
    std::vector<PlayerRepr> players_;
    std::unordered_map<std::string, std::vector<model::LostObject>> lost_objects_;
};

} // namespace serialization
