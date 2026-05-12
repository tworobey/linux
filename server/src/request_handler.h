#pragma once

#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <optional>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace fs = std::filesystem;

namespace endpoints {
    constexpr std::string_view MAPS       = "/api/v1/maps";
    constexpr std::string_view MAP_PREFIX = "/api/v1/maps/";
    constexpr std::string_view API_PREFIX = "/api/";
    constexpr std::string_view JOIN       = "/api/v1/game/join";
    constexpr std::string_view PLAYERS    = "/api/v1/game/players";
    constexpr std::string_view STATE      = "/api/v1/game/state";
    constexpr std::string_view ACTION     = "/api/v1/game/player/action";
    constexpr std::string_view TICK       = "/api/v1/game/tick";
}

namespace {

std::string UrlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            std::string hex = str.substr(i + 1, 2);
            char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += ch;
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string GetMimeType(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".htm" || ext == ".html") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".txt")  return "text/plain";
    if (ext == ".js")   return "text/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".svg")  return "image/svg+xml";

    return "application/octet-stream";
}

json::array SerializeRoads(const model::Map& map) {
    json::array roads;
    for (const auto& r : map.GetRoads()) {
        json::object road;
        road["x0"] = r.GetStart().x;
        road["y0"] = r.GetStart().y;
        if (r.IsHorizontal())
            road["x1"] = r.GetEnd().x;
        else
            road["y1"] = r.GetEnd().y;
        roads.push_back(road);
    }
    return roads;
}

json::array SerializeBuildings(const model::Map& map) {
    json::array buildings;
    for (const auto& b : map.GetBuildings()) {
        const auto& rect = b.GetBounds();
        buildings.push_back({
            {"x", rect.position.x},
            {"y", rect.position.y},
            {"w", rect.size.width},
            {"h", rect.size.height}
        });
    }
    return buildings;
}

json::array SerializeOffices(const model::Map& map) {
    json::array offices;
    for (const auto& o : map.GetOffices()) {
        offices.push_back({
            {"id",      *o.GetId()},
            {"x",       o.GetPosition().x},
            {"y",       o.GetPosition().y},
            {"offsetX", o.GetOffset().dx},
            {"offsetY", o.GetOffset().dy}
        });
    }
    return offices;
}

json::array SerializeMaps(const model::Game& game) {
    json::array arr;
    for (const auto& map : game.GetMaps()) {
        arr.push_back({
            {"id",   *map.GetId()},
            {"name", map.GetName()}
        });
    }
    return arr;
}

json::object SerializeMap(const model::Map& map) {
    json::object obj;
    obj["id"]        = *map.GetId();
    obj["name"]      = map.GetName();
    obj["roads"]     = SerializeRoads(map);
    obj["buildings"] = SerializeBuildings(map);
    obj["offices"]   = SerializeOffices(map);
    return obj;
}

json::object MakeError(std::string_view code, std::string_view message) {
    return {
        {"code",    code},
        {"message", message}
    };
}

} // anonymous namespace

// Извлекает токен из заголовка Authorization: Bearer <token>
// Возвращает токен или пустую строку если формат неверный
inline std::optional<std::string> TryExtractToken(
        const http::fields& fields) {
    auto it = fields.find(http::field::authorization);
    if (it == fields.end())
        return std::nullopt;

    std::string val = std::string(it->value());
    const std::string prefix = "Bearer ";
    if (val.size() < prefix.size() + 32 || val.substr(0, prefix.size()) != prefix)
        return std::nullopt;

    std::string token = val.substr(prefix.size());
    if (token.size() != 32 ||
        token.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        return std::nullopt;

    return token;
}

class RequestHandler {
public:
    RequestHandler(model::Game& game, fs::path static_root, bool tick_auto_mode = false)
        : game_(game)
        , static_root_(fs::weakly_canonical(std::move(static_root)))
        , tick_auto_mode_(tick_auto_mode) {
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {

        const std::string target(req.target());

        // -------- JOIN --------
        if (target == endpoints::JOIN) {
            if (req.method() != http::verb::post) {
                http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::allow, "POST");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidMethod", "Only POST method is expected"));
                res.prepare_payload();
                return send(std::move(res));
            }

            std::string userName, mapId;
            try {
                auto val  = json::parse(req.body());
                auto& obj = val.as_object();
                userName  = std::string(obj.at("userName").as_string());
                mapId     = std::string(obj.at("mapId").as_string());
            } catch (...) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidArgument", "Join game request parse error"));
                res.prepare_payload();
                return send(std::move(res));
            }

            if (userName.empty()) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidArgument", "Invalid name"));
                res.prepare_payload();
                return send(std::move(res));
            }

            if (!game_.FindMap(model::Map::Id(mapId))) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("mapNotFound", "Map not found"));
                res.prepare_payload();
                return send(std::move(res));
            }

            auto [token, playerId] = game_.JoinGame(userName, mapId);

            json::object resp;
            resp["authToken"] = token;
            resp["playerId"]  = playerId;

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(resp);
            res.prepare_payload();
            return send(std::move(res));
        }

        // -------- PLAYERS --------
        if (target == endpoints::PLAYERS) {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::allow, "GET, HEAD");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidMethod", "Invalid method"));
                res.prepare_payload();
                return send(std::move(res));
            }

            auto auth_it = req.find(http::field::authorization);
            if (auth_it == req.end()) {
                http::response<http::string_body> res{http::status::unauthorized, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidToken", "Authorization header is missing"));
                res.prepare_payload();
                return send(std::move(res));
            }

            std::string auth_val = std::string(auth_it->value());
            const std::string prefix = "Bearer ";
            if (auth_val.size() < prefix.size() + 32
                || auth_val.substr(0, prefix.size()) != prefix) {
                http::response<http::string_body> res{http::status::unauthorized, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidToken", "Authorization header is missing"));
                res.prepare_payload();
                return send(std::move(res));
            }

            std::string token = auth_val.substr(prefix.size());
            if (token.size() != 32
                || token.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
                http::response<http::string_body> res{http::status::unauthorized, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidToken", "Authorization header is missing"));
                res.prepare_payload();
                return send(std::move(res));
            }

            const model::Player* player = game_.FindPlayerByToken(token);
            if (!player) {
                http::response<http::string_body> res{http::status::unauthorized, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("unknownToken", "Player token has not been found"));
                res.prepare_payload();
                return send(std::move(res));
            }

            auto players = game_.GetPlayersOnMap(player->GetMapId());
            json::object result;
            for (auto* p : players) {
                json::object entry;
                entry["name"] = p->GetName();
                result[std::to_string(p->GetId())] = entry;
            }

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(result);
            res.prepare_payload();
            return send(std::move(res));
        }

// -------- TICK --------
        if (target == endpoints::TICK) {
            // Если сервер запущен с --tick-period, ручной тик запрещён
            if (tick_auto_mode_) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("badRequest", "Invalid endpoint"));
                res.prepare_payload();
                return send(std::move(res));
            }
            if (req.method() != http::verb::post) {
                http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::allow, "POST");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidMethod", "Invalid method"));
                res.prepare_payload();
                return send(std::move(res));
            }

            auto ct_it = req.find(http::field::content_type);
            if (ct_it == req.end() || std::string(ct_it->value()).find("application/json") == std::string::npos) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidArgument", "Invalid content type"));
                res.prepare_payload();
                return send(std::move(res));
            }

            double time_delta = 0.0;
            try {
                auto val = json::parse(req.body());
                auto& obj = val.as_object();
                // timeDelta должен быть числом (int или double)
                const auto& td = obj.at("timeDelta");
                if (td.is_int64())
                    time_delta = static_cast<double>(td.as_int64());
                else if (td.is_double())
                    time_delta = td.as_double();
                else
                    throw std::runtime_error("invalid timeDelta");
                if (time_delta < 0)
                    throw std::runtime_error("negative timeDelta");
            } catch (...) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidArgument", "Failed to parse tick request JSON"));
                res.prepare_payload();
                return send(std::move(res));
            }

            game_.Tick(time_delta);

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = "{}";
            res.prepare_payload();
            return send(std::move(res));
        }

// -------- ACTION --------
        if (target == endpoints::ACTION) {
            if (req.method() != http::verb::post) {
                http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::allow, "POST");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidMethod", "Invalid method"));
                res.prepare_payload();
                return send(std::move(res));
            }

            // Проверяем Content-Type
            auto ct_it = req.find(http::field::content_type);
            if (ct_it == req.end() || std::string(ct_it->value()).find("application/json") == std::string::npos) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidArgument", "Invalid content type"));
                res.prepare_payload();
                return send(std::move(res));
            }

            // Проверяем токен
            auto token_opt = TryExtractToken(req.base());
            if (!token_opt) {
                http::response<http::string_body> res{http::status::unauthorized, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidToken", "Authorization header is required"));
                res.prepare_payload();
                return send(std::move(res));
            }

            const model::Player* player = game_.FindPlayerByToken(*token_opt);
            if (!player) {
                http::response<http::string_body> res{http::status::unauthorized, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("unknownToken", "Player token has not been found"));
                res.prepare_payload();
                return send(std::move(res));
            }

            // Парсим тело запроса
            std::string move_str;
            try {
                auto val = json::parse(req.body());
                auto& obj = val.as_object();
                move_str = std::string(obj.at("move").as_string());
            } catch (...) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidArgument", "Failed to parse action"));
                res.prepare_payload();
                return send(std::move(res));
            }

            if (!game_.MovePlayer(*token_opt, move_str)) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "application/json");
                res.set(http::field::cache_control, "no-cache");
                res.body() = json::serialize(MakeError("invalidArgument", "Failed to parse action"));
                res.prepare_payload();
                return send(std::move(res));
            }

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = "{}";
            res.prepare_payload();
            return send(std::move(res));
        }

// -------- STATE --------
if (target == endpoints::STATE) {
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::allow, "GET, HEAD");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(MakeError("invalidMethod", "Invalid method"));
        res.prepare_payload();
        return send(std::move(res));
    }

    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(MakeError("invalidToken", "Authorization header is required"));
        res.prepare_payload();
        return send(std::move(res));
    }

    std::string auth_val = std::string(auth_it->value());
    const std::string prefix = "Bearer ";
    std::string token;
    if (auth_val.size() >= prefix.size() + 32 && auth_val.substr(0, prefix.size()) == prefix)
        token = auth_val.substr(prefix.size());

    if (token.size() != 32 ||
        token.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(MakeError("invalidToken", "Authorization header is required"));
        res.prepare_payload();
        return send(std::move(res));
    }

    const model::Player* player = game_.FindPlayerByToken(token);
    if (!player) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(MakeError("unknownToken", "Player token has not been found"));
        res.prepare_payload();
        return send(std::move(res));
    }

    auto players = game_.GetPlayersOnMap(player->GetMapId());
    json::object players_obj;
    for (auto* p : players) {
        const model::Dog* dog = p->GetDog();
        if (!dog) continue;

        json::array pos_arr, speed_arr;
        pos_arr.push_back(dog->GetPosition().x);
        pos_arr.push_back(dog->GetPosition().y);
        speed_arr.push_back(dog->GetSpeed().vx);
        speed_arr.push_back(dog->GetSpeed().vy);

        std::string dir;
        switch (dog->GetDirection()) {
            case model::DogDirection::NORTH: dir = "U"; break;
            case model::DogDirection::SOUTH: dir = "D"; break;
            case model::DogDirection::WEST:  dir = "L"; break;
            case model::DogDirection::EAST:  dir = "R"; break;
        }

        json::object entry;
        entry["pos"]   = pos_arr;
        entry["speed"] = speed_arr;
        entry["dir"]   = dir;
        players_obj[std::to_string(p->GetId())] = entry;
    }

    json::object result;
    result["players"] = players_obj;

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::cache_control, "no-cache");
    res.body() = json::serialize(result);
    res.prepare_payload();
    return send(std::move(res));
}

        // -------- MAPS LIST --------
        if (target == endpoints::MAPS) {
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = json::serialize(SerializeMaps(game_));
            res.prepare_payload();
            return send(std::move(res));
        }

        // -------- MAP BY ID --------
        if (target.rfind(endpoints::MAP_PREFIX, 0) == 0) {
            std::string id = target.substr(endpoints::MAP_PREFIX.size());
            const model::Map* map = game_.FindMap(model::Map::Id(id));

            if (!map) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(MakeError("mapNotFound", "Map not found"));
                res.prepare_payload();
                return send(std::move(res));
            }

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = json::serialize(SerializeMap(*map));
            res.prepare_payload();
            return send(std::move(res));
        }

        // -------- UNKNOWN API --------
        if (target.rfind(endpoints::API_PREFIX, 0) == 0) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = json::serialize(MakeError("badRequest", "Bad request"));
            res.prepare_payload();
            return send(std::move(res));
        }

        // -------- STATIC --------
        return HandleStatic(target, req, send);
    }

private:

    template <typename Body, typename Allocator, typename Send>
    void HandleStatic(const std::string& target,
                      const http::request<Body, http::basic_fields<Allocator>>& req,
                      Send&& send) {

        std::string path = UrlDecode(target);
        if (path == "/") path = "/index.html";

        fs::path full = fs::weakly_canonical(static_root_ / path.substr(1));

        auto rel = fs::relative(full, static_root_);
        if (rel.empty() || rel.string().rfind("..", 0) == 0) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Bad request";
            res.prepare_payload();
            return send(std::move(res));
        }

        if (fs::is_directory(full)) full /= "index.html";

        if (!fs::exists(full)) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Not found";
            res.prepare_payload();
            return send(std::move(res));
        }

        std::ifstream file(full, std::ios::binary);
        if (!file) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Not found";
            res.prepare_payload();
            return send(std::move(res));
        }

        std::string body((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, GetMimeType(full));
        res.body() = std::move(body);
        res.prepare_payload();
        return send(std::move(res));
    }

    model::Game& game_;
    fs::path static_root_;
    bool tick_auto_mode_ = false;
};

} // namespace http_handler
