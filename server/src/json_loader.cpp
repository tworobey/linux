#include "json_loader.h"

#include <boost/json.hpp>
#include <fstream>

namespace json = boost::json;

namespace json_loader {

    namespace {

        int ToInt(const json::value& v) {
            return static_cast<int>(v.as_int64());
        }

        model::Point MakePoint(int x, int y) {
            return { x, y };
        }

        void ParseRoads(const json::array& roads, model::Map& map) {
            for (const auto& road_val : roads) {
                const auto& r = road_val.as_object();

                model::Point start{
                    ToInt(r.at("x0")),
                    ToInt(r.at("y0"))
                };

                if (r.contains("x1")) {
                    map.AddRoad(model::Road(
                        model::Road::HORIZONTAL,
                        start,
                        ToInt(r.at("x1"))
                    ));
                }
                else {
                    map.AddRoad(model::Road(
                        model::Road::VERTICAL,
                        start,
                        ToInt(r.at("y1"))
                    ));
                }
            }
        }

        void ParseBuildings(const json::array& buildings, model::Map& map) {
            for (const auto& b_val : buildings) {
                const auto& b = b_val.as_object();

                model::Rectangle rect{
                    { ToInt(b.at("x")), ToInt(b.at("y")) },
                    { ToInt(b.at("w")), ToInt(b.at("h")) }
                };

                map.AddBuilding(model::Building(rect));
            }
        }

        void ParseOffices(const json::array& offices, model::Map& map) {
            for (const auto& o_val : offices) {
                const auto& o = o_val.as_object();

                map.AddOffice(model::Office(
                    model::Office::Id(std::string(o.at("id").as_string())),
                    { ToInt(o.at("x")), ToInt(o.at("y")) },
                    { ToInt(o.at("offsetX")), ToInt(o.at("offsetY")) }
                ));
            }
        }

        model::Map ParseMap(const json::object& map_obj) {
            model::Map map(
                model::Map::Id(std::string(map_obj.at("id").as_string())),
                std::string(map_obj.at("name").as_string())
            );

            // roads (обязательные)
            ParseRoads(map_obj.at("roads").as_array(), map);

            // buildings (опциональные)
            if (map_obj.contains("buildings")) {
                ParseBuildings(map_obj.at("buildings").as_array(), map);
            }

            // offices (опциональные)
            if (map_obj.contains("offices")) {
                ParseOffices(map_obj.at("offices").as_array(), map);
            }

            return map;
        }

    } // namespace

    //////////////////////////////////////////////////

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + json_path.string());

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    json::value doc;
    try {
        doc = json::parse(content);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse config JSON: " + std::string(e.what()));
    }
    const auto& obj = doc.as_object();

    model::Game game;

    if (obj.contains("defaultDogSpeed"))
        game.SetDefaultDogSpeed(obj.at("defaultDogSpeed").as_double());

    for (const auto& map_val : obj.at("maps").as_array()) {
        const auto& map_obj = map_val.as_object();
        model::Map map = ParseMap(map_obj);

        if (map_obj.contains("dogSpeed"))
            map.SetDogSpeed(map_obj.at("dogSpeed").as_double());
        else
            map.SetDogSpeed(game.GetDefaultDogSpeed());

        game.AddMap(std::move(map));
    }
    return game;
}
}  // namespace json_loader
