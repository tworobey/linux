#pragma once

#include <filesystem>

#include "model.h"
#include "extra_data.h"

namespace json_loader {

struct LoadResult {
    model::Game game;
    extra_data::MapExtraData extra_data;
};

LoadResult LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
