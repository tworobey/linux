#pragma once

#include "http_server.h"
#include "model.h"
#include "extra_data.h"

#include <boost/json.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

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

class RequestHandler {
public:
    RequestHandler(model::Game& game, extra_data::MapExtraData& extra_data,
                   fs::path static_root, bool tick_auto_mode = false);

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

private:
    model::Game& game_;
    extra_data::MapExtraData& extra_data_;
    fs::path static_root_;
    bool tick_auto_mode_ = false;
};

} // namespace http_handler

#include "request_handler_impl.h"
