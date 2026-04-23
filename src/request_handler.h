#pragma once
#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game)
            : game_{ game } {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

            const std::string target = std::string(req.target());

            // ---------- /api/v1/maps ----------
            if (target == "/api/v1/maps") {
                json::array arr;

                for (const auto& map : game_.GetMaps()) {
                    arr.push_back({
                        {"id", *map.GetId()},
                        {"name", map.GetName()}
                        });
                }

                http::response<http::string_body> res{ http::status::ok, req.version() };
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(arr);
                res.prepare_payload();

                return send(std::move(res));
            }

            // ---------- /api/v1/maps/{id} ----------
            if (target.starts_with("/api/v1/maps/")) {
                std::string id = target.substr(std::string("/api/v1/maps/").size());

                const model::Map* map = game_.FindMap(model::Map::Id(id));

                if (!map) {
                    json::object err{
                        {"code", "mapNotFound"},
                        {"message", "Map not found"}
                    };

                    http::response<http::string_body> res{ http::status::not_found, req.version() };
                    res.set(http::field::content_type, "application/json");
                    res.body() = json::serialize(err);
                    res.prepare_payload();

                    return send(std::move(res));
                }

                json::object obj;
                obj["id"] = *map->GetId();
                obj["name"] = map->GetName();

                // roads
                json::array roads;
                for (const auto& r : map->GetRoads()) {
                    json::object road;
                    road["x0"] = r.GetStart().x;
                    road["y0"] = r.GetStart().y;

                    if (r.IsHorizontal()) {
                        road["x1"] = r.GetEnd().x;
                    }
                    else {
                        road["y1"] = r.GetEnd().y;
                    }

                    roads.push_back(road);
                }
                obj["roads"] = roads;

                // buildings
                json::array buildings;
                for (const auto& b : map->GetBuildings()) {
                    const auto& rect = b.GetBounds();

                    buildings.push_back({
                        {"x", rect.position.x},
                        {"y", rect.position.y},
                        {"w", rect.size.width},
                        {"h", rect.size.height}
                        });
                }
                obj["buildings"] = buildings;

                // offices
                json::array offices;
                for (const auto& o : map->GetOffices()) {
                    offices.push_back({
                        {"id", *o.GetId()},
                        {"x", o.GetPosition().x},
                        {"y", o.GetPosition().y},
                        {"offsetX", o.GetOffset().dx},
                        {"offsetY", o.GetOffset().dy}
                        });
                }
                obj["offices"] = offices;

                http::response<http::string_body> res{ http::status::ok, req.version() };
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(obj);
                res.prepare_payload();

                return send(std::move(res));
            }

            // ---------- BAD REQUEST ----------
            if (target.starts_with("/api/")) {
                json::object err{
                    {"code", "badRequest"},
                    {"message", "Bad request"}
                };

                http::response<http::string_body> res{ http::status::bad_request, req.version() };
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(err);
                res.prepare_payload();

                return send(std::move(res));
            }
        }

    private:
        model::Game& game_;
    };

}  // namespace http_handler
