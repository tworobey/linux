#pragma once

#include "request_handler.h"
#include "db.h"
#include <fstream>
#include <algorithm>

namespace http_handler {

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
            {"x", rect.position.x}, {"y", rect.position.y},
            {"w", rect.size.width},  {"h", rect.size.height}
        });
    }
    return buildings;
}

json::array SerializeOffices(const model::Map& map) {
    json::array offices;
    for (const auto& o : map.GetOffices()) {
        offices.push_back({
            {"id", *o.GetId()}, {"x", o.GetPosition().x}, {"y", o.GetPosition().y},
            {"offsetX", o.GetOffset().dx}, {"offsetY", o.GetOffset().dy}
        });
    }
    return offices;
}

json::array SerializeMaps(const model::Game& game) {
    json::array arr;
    for (const auto& map : game.GetMaps())
        arr.push_back({ {"id", *map.GetId()}, {"name", map.GetName()} });
    return arr;
}

json::object SerializeMap(const model::Map& map, const extra_data::MapExtraData& extra_data) {
    json::object obj;
    obj["id"]        = *map.GetId();
    obj["name"]      = map.GetName();
    obj["roads"]     = SerializeRoads(map);
    obj["buildings"] = SerializeBuildings(map);
    obj["offices"]   = SerializeOffices(map);
    const auto* loot_types = extra_data.GetLootTypes(*map.GetId());
    obj["lootTypes"] = loot_types ? *loot_types : json::array{};
    return obj;
}

json::object MakeError(std::string_view code, std::string_view message) {
    return { {"code", code}, {"message", message} };
}

std::optional<std::string> TryExtractToken(const http::fields& fields) {
    constexpr size_t TOKEN_LENGTH = 32;
    auto it = fields.find(http::field::authorization);
    if (it == fields.end()) return std::nullopt;
    std::string val = std::string(it->value());
    const std::string prefix = "Bearer ";
    if (val.size() < prefix.size() + TOKEN_LENGTH || val.substr(0, prefix.size()) != prefix)
        return std::nullopt;
    std::string token = val.substr(prefix.size());
    if (token.size() != TOKEN_LENGTH ||
        token.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        return std::nullopt;
    return token;
}

} // anonymous namespace

inline RequestHandler::RequestHandler(model::Game& game, extra_data::MapExtraData& extra_data,
                                       fs::path static_root, bool tick_auto_mode,
                                       db::Database* database)
    : game_(game)
    , extra_data_(extra_data)
    , static_root_(fs::weakly_canonical(std::move(static_root)))
    , tick_auto_mode_(tick_auto_mode)
    , db_(database) {}

template <typename Body, typename Allocator, typename Send>
void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    const std::string target(req.target());

    auto make_response = [&](http::status status, std::string body, std::string content_type = "application/json") {
        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::content_type, content_type);
        res.set(http::field::cache_control, "no-cache");
        res.body() = std::move(body);
        res.prepare_payload();
        return res;
    };

    // JOIN
    if (target == endpoints::JOIN) {
        if (req.method() != http::verb::post)
            return send(make_response(http::status::method_not_allowed,
                json::serialize(MakeError("invalidMethod", "Only POST method is expected"))));
        std::string userName, mapId;
        try {
            auto val = json::parse(req.body());
            userName = std::string(val.as_object().at("userName").as_string());
            mapId    = std::string(val.as_object().at("mapId").as_string());
        } catch (...) {
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "Join game request parse error"))));
        }
        if (userName.empty())
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "Invalid name"))));
        if (!game_.FindMap(model::Map::Id(mapId)))
            return send(make_response(http::status::not_found,
                json::serialize(MakeError("mapNotFound", "Map not found"))));
        auto [token, playerId] = game_.JoinGame(userName, mapId);
        return send(make_response(http::status::ok,
            json::serialize(json::object{{"authToken", token}, {"playerId", playerId}})));
    }

    // PLAYERS
    if (target == endpoints::PLAYERS) {
        if (req.method() != http::verb::get && req.method() != http::verb::head)
            return send(make_response(http::status::method_not_allowed,
                json::serialize(MakeError("invalidMethod", "Invalid method"))));
        auto token_opt = TryExtractToken(req.base());
        if (!token_opt)
            return send(make_response(http::status::unauthorized,
                json::serialize(MakeError("invalidToken", "Authorization header is missing"))));
        const model::Player* player = game_.FindPlayerByToken(*token_opt);
        if (!player)
            return send(make_response(http::status::unauthorized,
                json::serialize(MakeError("unknownToken", "Player token has not been found"))));
        json::object result;
        for (auto* p : game_.GetPlayersOnMap(player->GetMapId()))
            result[std::to_string(p->GetId())] = json::object{{"name", p->GetName()}};
        return send(make_response(http::status::ok, json::serialize(result)));
    }

    // TICK
    if (target == endpoints::TICK) {
        if (tick_auto_mode_)
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("badRequest", "Invalid endpoint"))));
        if (req.method() != http::verb::post)
            return send(make_response(http::status::method_not_allowed,
                json::serialize(MakeError("invalidMethod", "Invalid method"))));
        auto ct_it = req.find(http::field::content_type);
        if (ct_it == req.end() || std::string(ct_it->value()).find("application/json") == std::string::npos)
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "Invalid content type"))));
        double time_delta = 0.0;
        try {
            auto val = json::parse(req.body());
            const auto& td = val.as_object().at("timeDelta");
            if (td.is_int64())       time_delta = static_cast<double>(td.as_int64());
            else if (td.is_double()) time_delta = td.as_double();
            else throw std::runtime_error("invalid");
            if (time_delta < 0) throw std::runtime_error("negative");
        } catch (...) {
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "Failed to parse tick request JSON"))));
        }
        game_.Tick(time_delta);
        return send(make_response(http::status::ok, "{}"));
    }

    // ACTION
    if (target == endpoints::ACTION) {
        if (req.method() != http::verb::post)
            return send(make_response(http::status::method_not_allowed,
                json::serialize(MakeError("invalidMethod", "Invalid method"))));
        auto ct_it = req.find(http::field::content_type);
        if (ct_it == req.end() || std::string(ct_it->value()).find("application/json") == std::string::npos)
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "Invalid content type"))));
        auto token_opt = TryExtractToken(req.base());
        if (!token_opt)
            return send(make_response(http::status::unauthorized,
                json::serialize(MakeError("invalidToken", "Authorization header is required"))));
        if (!game_.FindPlayerByToken(*token_opt))
            return send(make_response(http::status::unauthorized,
                json::serialize(MakeError("unknownToken", "Player token has not been found"))));
        std::string move_str;
        try {
            auto val = json::parse(req.body());
            move_str = std::string(val.as_object().at("move").as_string());
        } catch (...) {
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "Failed to parse action"))));
        }
        if (!game_.MovePlayer(*token_opt, move_str))
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "Failed to parse action"))));
        return send(make_response(http::status::ok, "{}"));
    }

    // STATE
    if (target == endpoints::STATE) {
        if (req.method() != http::verb::get && req.method() != http::verb::head)
            return send(make_response(http::status::method_not_allowed,
                json::serialize(MakeError("invalidMethod", "Invalid method"))));
        auto token_opt = TryExtractToken(req.base());
        if (!token_opt)
            return send(make_response(http::status::unauthorized,
                json::serialize(MakeError("invalidToken", "Authorization header is required"))));
        const model::Player* player = game_.FindPlayerByToken(*token_opt);
        if (!player)
            return send(make_response(http::status::unauthorized,
                json::serialize(MakeError("unknownToken", "Player token has not been found"))));

        json::object players_obj;
        for (auto* p : game_.GetPlayersOnMap(player->GetMapId())) {
            const model::Dog* dog = p->GetDog();
            if (!dog) continue;
            json::array pos_arr{dog->GetPosition().x, dog->GetPosition().y};
            json::array spd_arr{dog->GetSpeed().vx, dog->GetSpeed().vy};
            std::string dir;
            switch (dog->GetDirection()) {
                case model::DogDirection::NORTH: dir = "U"; break;
                case model::DogDirection::SOUTH: dir = "D"; break;
                case model::DogDirection::WEST:  dir = "L"; break;
                case model::DogDirection::EAST:  dir = "R"; break;
            }
            json::array bag_arr;
            for (const auto& item : dog->GetBag())
                bag_arr.push_back(json::object{{"id", item.id}, {"type", item.type}});
            players_obj[std::to_string(p->GetId())] = json::object{
                {"pos", pos_arr}, {"speed", spd_arr}, {"dir", dir},
                {"bag", bag_arr}, {"score", dog->GetScore()}
            };
        }

        json::object lost_obj;
        for (const auto& lo : game_.GetLostObjects(player->GetMapId()))
            lost_obj[std::to_string(lo.id)] = json::object{
                {"type", lo.type}, {"pos", json::array{lo.pos.x, lo.pos.y}}
            };

        return send(make_response(http::status::ok,
            json::serialize(json::object{{"players", players_obj}, {"lostObjects", lost_obj}})));
    }

    // RECORDS
    if (target.rfind(std::string(endpoints::RECORDS), 0) == 0) {
        if (req.method() != http::verb::get && req.method() != http::verb::head)
            return send(make_response(http::status::method_not_allowed,
                json::serialize(MakeError("invalidMethod", "Invalid method"))));

        if (!db_)
            return send(make_response(http::status::internal_server_error,
                json::serialize(MakeError("serverError", "Database not available"))));

        // Парсим параметры start и maxItems из URL
        int start = 0;
        int max_items = 100;

        std::string query;
        auto pos = target.find('?');
        if (pos != std::string::npos)
            query = target.substr(pos + 1);

        auto parse_param = [&query](const std::string& name) -> std::optional<int> {
            auto it = query.find(name + "=");
            if (it == std::string::npos) return std::nullopt;
            it += name.size() + 1;
            auto end = query.find('&', it);
            try {
                return std::stoi(query.substr(it, end == std::string::npos ? end : end - it));
            } catch (...) {
                return std::nullopt;
            }
        };

        if (auto s = parse_param("start")) start = *s;
        if (auto m = parse_param("maxItems")) max_items = *m;

        if (max_items > 100)
            return send(make_response(http::status::bad_request,
                json::serialize(MakeError("invalidArgument", "maxItems must not exceed 100"))));

        try {
            auto records = db_->GetRecords(start, max_items);
            json::array arr;
            for (const auto& r : records) {
                json::object entry;
                entry["name"]     = r.name;
                entry["score"]    = r.score;
                entry["playTime"] = r.play_time;
                arr.push_back(entry);
            }
            return send(make_response(http::status::ok, json::serialize(arr)));
        } catch (const std::exception& e) {
            return send(make_response(http::status::internal_server_error,
                json::serialize(MakeError("serverError", e.what()))));
        }
    }

    // MAPS LIST
    if (target == endpoints::MAPS) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.body() = json::serialize(SerializeMaps(game_));
        res.prepare_payload();
        return send(std::move(res));
    }

    // MAP BY ID
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
        res.body() = json::serialize(SerializeMap(*map, extra_data_));
        res.prepare_payload();
        return send(std::move(res));
    }

    // UNKNOWN API
    if (target.rfind(endpoints::API_PREFIX, 0) == 0)
        return send(make_response(http::status::bad_request,
            json::serialize(MakeError("badRequest", "Bad request"))));

    // STATIC
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
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, GetMimeType(full));
    res.body() = std::move(body);
    res.prepare_payload();
    return send(std::move(res));
}

} // namespace http_handler
