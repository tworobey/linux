#pragma once

#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>
#include <string>
#include <string_view>
#include <iostream>
#include <exception>

namespace http_handler {

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    //////////////////////////////////////////////////
    // ENDPOINTS
    //////////////////////////////////////////////////

    namespace endpoints {
        constexpr std::string_view MAPS = "/api/v1/maps";
        constexpr std::string_view MAP_PREFIX = "/api/v1/maps/";
        constexpr std::string_view API_PREFIX = "/api/";
    }

    //////////////////////////////////////////////////
    // SERIALIZATION HELPERS
    //////////////////////////////////////////////////

    namespace {

        json::array SerializeMaps(const model::Game& game) {
            json::array arr;

            for (const auto& map : game.GetMaps()) {
                arr.push_back({
                    {"id", *map.GetId()},
                    {"name", map.GetName()}
                    });
            }

            return arr;
        }

        json::object SerializeMap(const model::Map& map) {
            json::object obj;
            obj["id"] = *map.GetId();
            obj["name"] = map.GetName();

            json::array roads;
            for (const auto& r : map.GetRoads()) {
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

            json::array buildings;
            for (const auto& b : map.GetBuildings()) {
                const auto& rect = b.GetBounds();

                buildings.push_back({
                    {"x", static_cast<int>(rect.position.x)},
                    {"y", static_cast<int>(rect.position.y)},
                    {"w", static_cast<int>(rect.size.width)},
                    {"h", static_cast<int>(rect.size.height)}
                    });
            }
            obj["buildings"] = buildings;

            json::array offices;
            for (const auto& o : map.GetOffices()) {
                offices.push_back({
                    {"id", *o.GetId()},
                    {"x", o.GetPosition().x},
                    {"y", o.GetPosition().y},
                    {"offsetX", o.GetOffset().dx},
                    {"offsetY", o.GetOffset().dy}
                    });
            }
            obj["offices"] = offices;

            return obj;
        }

        json::object MakeError(std::string_view code, std::string_view message) {
            return {
                {"code", code},
                {"message", message}
            };
        }

    } // namespace

    //////////////////////////////////////////////////
    // REQUEST HANDLER
    //////////////////////////////////////////////////

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game)
            : game_(game) {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
            Send&& send) {

            try {
                const std::string target(req.target());

                std::cerr << "REQUEST: " << target << std::endl;

                // ---------- /api/v1/maps ----------
                if (target == endpoints::MAPS) {
                    http::response<http::string_body> res{ http::status::ok, req.version() };
                    res.set(http::field::content_type, "application/json");
                    res.body() = json::serialize(SerializeMaps(game_));
                    res.prepare_payload();
                    return send(std::move(res));
                }

                // ---------- /api/v1/maps/{id} ----------
                if (target.starts_with(endpoints::MAP_PREFIX)) {
                    std::string id = target.substr(endpoints::MAP_PREFIX.size());

                    const model::Map* map = game_.FindMap(model::Map::Id(id));

                    if (!map) {
                        http::response<http::string_body> res{ http::status::not_found, req.version() };
                        res.set(http::field::content_type, "application/json");
                        res.body() = json::serialize(MakeError("mapNotFound", "Map not found"));
                        res.prepare_payload();
                        return send(std::move(res));
                    }

                    http::response<http::string_body> res{ http::status::ok, req.version() };
                    res.set(http::field::content_type, "application/json");
                    res.body() = json::serialize(SerializeMap(*map));
                    res.prepare_payload();
                    return send(std::move(res));
                }

                // ---------- /api/... ----------
                if (target.starts_with(endpoints::API_PREFIX)) {
                    http::response<http::string_body> res{ http::status::bad_request, req.version() };
                    res.set(http::field::content_type, "application/json");
                    res.body() = json::serialize(MakeError("badRequest", "Bad request"));
                    res.prepare_payload();
                    return send(std::move(res));
                }

                // ---------- fallback ----------
                http::response<http::string_body> res{ http::status::not_found, req.version() };
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(MakeError("notFound", "Route not found"));
                res.prepare_payload();
                return send(std::move(res));

            }
            catch (const std::exception& e) {
                std::cerr << "HANDLER EXCEPTION: " << e.what() << std::endl;

                http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(MakeError("internalError", "Server error"));
                res.prepare_payload();
                return send(std::move(res));
            }
        }

    private:
        model::Game& game_;
    };

} // namespace http_handler