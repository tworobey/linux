#include "json_loader.h"

#include <boost/json.hpp>
#include <fstream>

namespace json = boost::json;

namespace json_loader {

    model::Game LoadGame(const std::filesystem::path& json_path) {
        std::ifstream file(json_path);
        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        json::value doc = json::parse(content);
        auto obj = doc.as_object();

        model::Game game;

        for (const auto& map_val : obj.at("maps").as_array()) {
            const auto& map_obj = map_val.as_object();

            model::Map map(
                model::Map::Id(std::string(map_obj.at("id").as_string())),
                std::string(map_obj.at("name").as_string())
            );

            // roads
            for (const auto& road_val : map_obj.at("roads").as_array()) {
                const auto& r = road_val.as_object();

                model::Point start{
                    (int)r.at("x0").as_int64(),
                    (int)r.at("y0").as_int64()
                };

                if (r.contains("x1")) {
                    map.AddRoad(model::Road(
                        model::Road::HORIZONTAL,
                        start,
                        (int)r.at("x1").as_int64()
                    ));
                }
                else {
                    map.AddRoad(model::Road(
                        model::Road::VERTICAL,
                        start,
                        (int)r.at("y1").as_int64()
                    ));
                }
            }

            // buildings
            if (map_obj.contains("buildings")) {
                for (const auto& b_val : map_obj.at("buildings").as_array()) {
                    const auto& b = b_val.as_object();

                    model::Rectangle rect{
                        { (int)b.at("x").as_int64(), (int)b.at("y").as_int64() },
                        { (int)b.at("w").as_int64(), (int)b.at("h").as_int64() }
                    };

                    map.AddBuilding(model::Building(rect));
                }
            }

            // offices
            if (map_obj.contains("offices")) {
                for (const auto& o_val : map_obj.at("offices").as_array()) {
                    const auto& o = o_val.as_object();

                    map.AddOffice(model::Office(
                        model::Office::Id(std::string(o.at("id").as_string())),
                        { (int)o.at("x").as_int64(), (int)o.at("y").as_int64() },
                        { (int)o.at("offsetX").as_int64(), (int)o.at("offsetY").as_int64() }
                    ));
                }
            }

            game.AddMap(std::move(map));
        }

        return game;
    }

}  // namespace json_loader